/**********************************************************************

  Audacity: A Digital Audio Editor

  Export.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Export
\brief Main class to control the export function.

*//****************************************************************//**

\class ExportType
\brief Container for information about supported export types.

*//****************************************************************//**

\class ExportMixerDialog
\brief Dialog for advanced mixing.

*//****************************************************************//**

\class ExportMixerPanel
\brief Panel that displays mixing for advanced mixing option.

*//********************************************************************/


#include "Export.h"

#include <wx/bmpbuttn.h>
#include <wx/dcclient.h>
#include <wx/file.h>
#include <wx/filectrl.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/dcmemory.h>
#include <wx/window.h>

#include "sndfile.h"

// Tenacity libraries
#include <lib-basic-ui/BasicUI.h>
#include <lib-files/FileNames.h>
#include <lib-files/wxFileNameWrapper.h>
#include <lib-preferences/Prefs.h>

#include "ExportDialog.h"
#include "../theme/AllThemeResources.h"
#include "Mix.h"
#include "../prefs/ImportExportPrefs.h"
#include "Project.h"
#include "../ProjectHistory.h"
#include "../ProjectSettings.h"
#include "../ProjectWindow.h"
#include "../ProjectWindows.h"
#include "../shuttle/ShuttleGui.h"
#include "../Tags.h"
#include "../theme/Theme.h"
#include "../WaveTrack.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/Warning.h"
#include "../widgets/HelpSystem.h"
#include "../AColor.h"
#include "widgets/HelpSystem.h"
#include "widgets/ProgressDialog.h"

//----------------------------------------------------------------------------
// Export
//----------------------------------------------------------------------------


wxDEFINE_EVENT(AUDACITY_FILE_SUFFIX_EVENT, wxCommandEvent);

BEGIN_EVENT_TABLE(Exporter, wxEvtHandler)
   EVT_FILECTRL_FILTERCHANGED(wxID_ANY, Exporter::OnFilterChanged)
   EVT_BUTTON(wxID_HELP, Exporter::OnHelp)
END_EVENT_TABLE()

namespace {
const auto PathStart = wxT("Exporters");

static Registry::GroupItem &sRegistry()
{
   static Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}

struct ExporterItem final : Registry::SingleItem {
   ExporterItem(
      const Identifier &id, const Exporter::ExportPluginFactory &factory )
      : SingleItem{ id }
      , mFactory{ factory }
   {}

   Exporter::ExportPluginFactory mFactory;
};

   using ExportPluginFactories = std::vector< Exporter::ExportPluginFactory >;
   ExportPluginFactories &sFactories()
   {
      static ExportPluginFactories theList;
      return theList;
   }
}

Exporter::RegisteredExportPlugin::RegisteredExportPlugin(
   const Identifier &id,
   const ExportPluginFactory &factory,
   const Registry::Placement &placement )
{
   if ( factory )
      Registry::RegisterItem( sRegistry(), placement,
         std::make_unique< ExporterItem >( id, factory ) );
}

Exporter::Exporter( TenacityProject &project )
: mProject{ &project }
{
   using namespace Registry;
   static OrderingPreferenceInitializer init{
      PathStart,
      { {wxT(""), wxT("PCM,MP3,OGG,FLAC,MP2,CommandLine,FFmpeg") } },
   };

   mMixerSpec = NULL;
   mBook = NULL;

   // build the list of export plugins.
   for ( const auto &factory : sFactories() )
      mPlugins.emplace_back( factory() );

   struct MyVisitor final : Visitor {
      MyVisitor()
      {
         // visit the registry to collect the plug-ins properly
         // sorted
         TransparentGroupItem<> top{ PathStart };
         Registry::Visit( *this, &top, &sRegistry() );
      }

      void Visit( SingleItem &item, const Path &path ) override
      {
         mPlugins.emplace_back(
            static_cast<ExporterItem&>( item ).mFactory() );
      }

      ExportPluginArray mPlugins;
   } visitor;

   mPlugins.swap( visitor.mPlugins );

   SetFileDialogTitle( XO("Export Audio") );
}

Exporter::~Exporter()
{
}

void Exporter::OnHelp(wxCommandEvent& /* evt */)
{
   wxWindow * pWin = FindProjectFrame( mProject );
   HelpSystem::ShowHelp(pWin, L"File_Export_Dialog", true);
}

void Exporter::SetFileDialogTitle( const TranslatableString & DialogTitle )
{
   // The default title is "Export File"
   mFileDialogTitle = DialogTitle;
}

int Exporter::FindFormatIndex(int exportindex)
{
   int c = 0;
   for (const auto &pPlugin : mPlugins)
   {
      for (int j = 0; j < pPlugin->GetFormatCount(); j++)
      {
         if (exportindex == c) return j;
         c++;
      }
   }
   return 0;
}

const ExportPluginArray &Exporter::GetPlugins()
{
   return mPlugins;
}

bool Exporter::DoEditMetadata(TenacityProject &project,
   const TranslatableString &title,
   const TranslatableString &shortUndoDescription, bool force)
{
   auto &settings = ProjectSettings::Get( project );
   auto &tags = Tags::Get( project );

   // Back up my tags
   // Tags (artist name, song properties, MP3 ID3 info, etc.)
   // The structure may be shared with undo history entries
   // To keep undo working correctly, always replace this with a NEW duplicate
   // BEFORE doing any editing of it!
   auto newTags = tags.Duplicate();

   if (newTags->ShowEditDialog(&GetProjectFrame( project ), title, force)) {
      if (tags != *newTags) {
         // Commit the change to project state only now.
         Tags::Set( project, newTags );
         ProjectHistory::Get( project ).PushState( title, shortUndoDescription);
      }
      bool bShowInFuture;
      gPrefs->Read(wxT("/AudioFiles/ShowId3Dialog"), &bShowInFuture, true);
      settings.SetShowId3Dialog( bShowInFuture );
      return true;
   }

   return false;
}

bool Exporter::Process(bool selectedOnly, double t0, double t1)
{
   // Save parms
   mSelectedOnly = selectedOnly;
   mT0 = t0;
   mT1 = t1;

   // Gather track information
   if (!ExamineTracks()) {
      return false;
   }

   // Ask user for file name
   if (!GetFilename()) {
      return false;
   }

   // Check for down mixing
   if (!CheckMix()) {
      return false;
   }

   // Let user edit MetaData
   if (mPlugins[mFormat]->GetCanMetaData(mSubFormat)) {
      if (!DoEditMetadata( *mProject,
         XO("Edit Metadata Tags"), XO("Exported Tags"),
         ProjectSettings::Get( *mProject ).GetShowId3Dialog())) {
         return false;
      }
   }

   // Ensure filename doesn't interfere with project files.
   if (!CheckFilename()) {
      return false;
   }

   // Export the tracks
   bool success = ExportTracks();

   // Get rid of mixerspec
   mMixerSpec.reset();

   if (success) {
      if (mFormatName.empty()) {
         gPrefs->Write(wxT("/Export/Format"), mPlugins[mFormat]->GetFormat(mSubFormat));
      }

      FileNames::UpdateDefaultPath(FileNames::Operation::Export, mFilename.GetPath());
   }

   return success;
}

bool Exporter::Process(unsigned numChannels,
                       const FileExtension &type, const wxString & filename,
                       bool selectedOnly, double t0, double t1)
{
   // Save parms
   mChannels = numChannels;
   mFilename = filename;
   mSelectedOnly = selectedOnly;
   mT0 = t0;
   mT1 = t1;
   mActualName = mFilename;

   int i = -1;
   for (const auto &pPlugin : mPlugins) {
      ++i;
      for (int j = 0; j < pPlugin->GetFormatCount(); j++)
      {
         if (pPlugin->GetFormat(j).IsSameAs(type, false))
         {
            mFormat = i;
            mSubFormat = j;
            return CheckFilename() && ExportTracks();
         }
      }
   }

   return false;
}

bool Exporter::ExamineTracks()
{
   // Init
   mNumSelected = 0;
   mNumLeft = 0;
   mNumRight = 0;
   mNumMono = 0;

   // First analyze the selected audio, perform sanity checks, and provide
   // information as appropriate.

   // Tally how many are right, left, mono, and make sure at
   // least one track is selected (if selectedOnly==true)

   double earliestBegin = mT1;
   double latestEnd = mT0;

   auto &tracks = TrackList::Get( *mProject );

   bool anySolo = !(( tracks.Any<const WaveTrack>() + &WaveTrack::GetSolo ).empty());

   for (auto tr :
         tracks.Any< const WaveTrack >()
            + ( mSelectedOnly ? &Track::IsSelected : &Track::Any )
            - ( anySolo ? &WaveTrack::GetNotSolo : &WaveTrack::GetMute)
   ) {
      mNumSelected++;

      if (tr->GetChannel() == Track::LeftChannel) {
         mNumLeft++;
      }
      else if (tr->GetChannel() == Track::RightChannel) {
         mNumRight++;
      }
      else if (tr->GetChannel() == Track::MonoChannel) {
         // It's a mono channel, but it may be panned
         float pan = tr->GetPan();

         if (pan == -1.0)
            mNumLeft++;
         else if (pan == 1.0)
            mNumRight++;
         else if (pan == 0)
            mNumMono++;
         else {
            // Panned partially off-center. Mix as stereo.
            mNumLeft++;
            mNumRight++;
         }
      }

      if (tr->GetOffset() < earliestBegin) {
         earliestBegin = tr->GetOffset();
      }

      if (tr->GetEndTime() > latestEnd) {
         latestEnd = tr->GetEndTime();
      }
   }

   if (mNumSelected == 0) {
      TranslatableString message;
      if(mSelectedOnly)
         message = XO("All selected audio is muted.");
      else
         message = XO("All audio is muted.");
      ShowExportErrorDialog(
         ":576",
         message);
      return false;
   }

   // The skipping of silent space could be cleverer and take 
   // into account clips.
   // As implemented now, it can only skip initial silent space that 
   // has no clip before it, and terminal silent space that has no clip 
   // after it.
   if (mT0 < earliestBegin){
      // Bug 1904 
      // Previously we always skipped initial silent space.
      // Now skipping it is an opt-in option.
      bool skipSilenceAtBeginning;
      gPrefs->Read(wxT("/AudioFiles/SkipSilenceAtBeginning"),
                                      &skipSilenceAtBeginning, false);
      if (skipSilenceAtBeginning)
         mT0 = earliestBegin;
   }

   // We still skip silent space at the end
   if (mT1 > latestEnd)
      mT1 = latestEnd;

   return true;
}

#include <wx/log.h>

bool Exporter::GetFilename()
{
   mFormat = -1;

   FileNames::FileTypes fileTypes;
   auto defaultFormat = mFormatName;
   if( defaultFormat.empty() )
      defaultFormat = gPrefs->Read(wxT("/Export/Format"),
                                         wxT("WAV"));

   mFilterIndex = 0;

   {
      int i = -1;
      for (const auto &pPlugin : mPlugins) {
         ++i;
         for (int j = 0; j < pPlugin->GetFormatCount(); j++)
         {
            auto mask = pPlugin->GetMask(j);
            fileTypes.insert( fileTypes.end(), mask.begin(), mask.end() );
            if (mPlugins[i]->GetFormat(j) == defaultFormat) {
               mFormat = i;
               mSubFormat = j;
            }
            if (mFormat == -1) mFilterIndex++;
         }
      }
   }
   if (mFormat == -1)
   {
      mFormat = 0;
      mFilterIndex = 0;
      mSubFormat = 0;
   }
   wxString defext = mPlugins[mFormat]->GetExtension(mSubFormat).Lower();

   //Bug 1304: Set a default path if none was given.  For Export.
   mFilename.SetPath(FileNames::FindDefaultPath(FileNames::Operation::Export));
   mFilename.SetName(mProject->GetProjectName());
   if (mFilename.GetName().empty())
      mFilename.SetName(_("untitled"));
   int index = 0;
   while (true) {
      // Must reset each iteration
      mBook = NULL;

      {
         auto useFileName = mFilename;
         if (!useFileName.HasExt())
            useFileName.SetExt(defext);

         FileDialogWrapper fd(
            ProjectWindow::Find( mProject ),
            mFileDialogTitle,
            mFilename.GetPath(),
            useFileName.GetFullName(),
            fileTypes,
            wxFD_SAVE | wxRESIZE_BORDER
         );

         mDialog = &fd;

         fd.SetFilterIndex(mFilterIndex);

         int result = fd.ShowModal();
         if (result == wxID_CANCEL) {
            return false;
         }

         mFilename = fd.GetPath();
         if (mFilename == wxT("")) {
            return false;
         }

         mFormat = fd.GetFilterIndex();
         mFilterIndex = fd.GetFilterIndex();
      }

      {
         int c = 0;
         int i = -1;
         for (const auto &pPlugin : mPlugins)
         {
            ++i;
            for (int j = 0; j < pPlugin->GetFormatCount(); j++)
            {
               if (mFilterIndex == c)
               {
                  mFormat = i;
                  mSubFormat = j;
               }
               c++;
            }
         }
      }

      const auto ext = mFilename.GetExt();
      defext = mPlugins[mFormat]->GetExtension(mSubFormat).Lower();

      //
      // Check the extension - add the default if it's not there,
      // and warn user if it's abnormal.
      //
      if (ext.empty()) {
         //
         // Make sure the user doesn't accidentally save the file
         // as an extension with no name, like just plain ".wav".
         //
         if (mFilename.GetName().Left(1) == wxT(".")) {
            auto prompt =
               XO("Are you sure you want to export the file as \"%s\"?\n")
                  .Format( mFilename.GetFullName() );

            int action = AudacityMessageBox(
               prompt,
               XO("Warning"),
               wxYES_NO | wxICON_EXCLAMATION);
            if (action != wxYES) {
               continue;
            }
         }

         mFilename.SetExt(defext);
      }

      if (!mPlugins[mFormat]->CheckFileName(mFilename, mSubFormat))
      {
         continue;
      }
      else if (!ext.empty() && !mPlugins[mFormat]->IsExtension(ext,mSubFormat) && ext.CmpNoCase(defext)) {
         auto prompt = XO("You are about to export a %s file with the name \"%s\".\n\nNormally these files end in \".%s\", and some programs will not open files with nonstandard extensions.\n\nAre you sure you want to export the file under this name?")
               .Format(mPlugins[mFormat]->GetFormat(mSubFormat),
                       mFilename.GetFullName(),
                       defext);

         int action = AudacityMessageBox(
            prompt,
            XO("Warning"),
            wxYES_NO | wxICON_EXCLAMATION);
         if (action != wxYES) {
            continue;
         }
      }

      if (mFilename.GetFullPath().length() >= 256) {
         AudacityMessageBox(
            XO( "Sorry, pathnames longer than 256 characters not supported.") );
         continue;
      }

// FIXME: is this already handled by wxFileDialog?
#if !defined(__WXMAC__)
      if (mFilename.FileExists()) {
         auto prompt = XO("A file named \"%s\" already exists. Replace?")
            .Format( mFilename.GetFullPath() );

         int action = AudacityMessageBox(
            prompt,
            XO("Warning"),
            wxYES_NO | wxICON_EXCLAMATION);
         if (action != wxYES) {
            continue;
         }
      }
#endif

      ExportOptionsDialog optionsDialog(mPlugins);
      optionsDialog.SetIndex(mFilterIndex);
      int result = optionsDialog.ShowModal();

      if (result == wxID_CANCEL)
      {
         return false;
      }

      break;
   }

   return true;
}

//
// For safety, if the file already exists it stores the filename
// the user wants in actualName, and returns a temporary file name.
// The calling function should rename the file when it's successfully
// exported.
//
bool Exporter::CheckFilename()
{
   //
   // To be even safer, return a temporary file name based
   // on this one...
   //

   mActualName = mFilename;

   int suffix = 0;
   while (mFilename.FileExists()) {
      mFilename.SetName(mActualName.GetName() +
                        wxString::Format(wxT("%d"), suffix));
      suffix++;
   }

   return true;
}

void Exporter::DisplayOptions(int index)
{
   int c = 0;
   int mf = -1, msf = -1;
   int i = -1;
   for (const auto &pPlugin : mPlugins)
   {
      ++i;
      for (int j = 0; j < pPlugin->GetFormatCount(); j++)
      {
         if (index == c)
         {
            mf = i;
            msf = j;
         }
         c++;
      }
   }
   // This shouldn't happen...
   if (index >= c) {
      return;
   }

#if defined(__WXMSW__)
   mPlugins[mf]->DisplayOptions( FindProjectFrame( mProject ), msf);
#else
   mPlugins[mf]->DisplayOptions(mDialog, msf);
#endif
}

bool Exporter::CheckMix(bool prompt /*= true*/ )
{
   // Clean up ... should never happen
   mMixerSpec.reset();

   // Determine if exported file will be stereo or mono or multichannel,
   // and if mixing will occur.

   auto downMix = ImportExportPrefs::ExportDownMixSetting.ReadEnum();
   int exportedChannels = mPlugins[mFormat]->SetNumExportChannels();

   if (downMix) {
      if (mNumRight > 0 || mNumLeft > 0) {
         mChannels = 2;
      }
      else {
         mChannels = 1;
      }
      mChannels = std::min(mChannels,
                           mPlugins[mFormat]->GetMaxChannels(mSubFormat));

      auto numLeft =  mNumLeft + mNumMono;
      auto numRight = mNumRight + mNumMono;

      if (numLeft > 1 || numRight > 1 || mNumLeft + mNumRight + mNumMono > mChannels) {
         wxString exportFormat = mPlugins[mFormat]->GetFormat(mSubFormat);
         if (exportFormat != wxT("CL") && exportFormat != wxT("FFMPEG") && exportedChannels == -1)
            exportedChannels = mChannels;

         if (prompt) {
            auto pWindow = ProjectWindow::Find(mProject);
            if (exportedChannels == 1) {
               if (ShowWarningDialog(pWindow,
                  wxT("MixMono"),
                  XO("Your tracks will be mixed down and exported as one mono file."),
                  true) == wxID_CANCEL)
                  return false;
            }
            else if (exportedChannels == 2) {
               if (ShowWarningDialog(pWindow,
                  wxT("MixStereo"),
                  XO("Your tracks will be mixed down and exported as one stereo file."),
                  true) == wxID_CANCEL)
                  return false;
            }
            else {
               if (ShowWarningDialog(pWindow,
                  wxT("MixUnknownChannels"),
                  XO("Your tracks will be mixed down to one exported file according to the encoder settings."),
                  true) == wxID_CANCEL)
                  return false;
            }
         }
      }
   }
   else
   {
      if (exportedChannels < 0)
         exportedChannels = mPlugins[mFormat]->GetMaxChannels(mSubFormat);

      ExportMixerDialog md(&TrackList::Get( *mProject ),
                           mSelectedOnly,
                           exportedChannels,
                           NULL,
                           1,
                           XO("Advanced Mixing Options"));
      if (prompt) {
         if (md.ShowModal() != wxID_OK) {
            return false;
         }
      }

      mMixerSpec = std::make_unique<MixerSpec>(*(md.GetMixerSpec()));
      mChannels = mMixerSpec->GetNumChannels();
   }

   return true;
}

bool Exporter::ExportTracks()
{
   // Keep original in case of failure
   if (mActualName != mFilename) {
      ::wxRenameFile(mActualName.GetFullPath(), mFilename.GetFullPath());
   }

   bool success = false;

   auto cleanup = finally( [&] {
      if (mActualName != mFilename) {
         // Remove backup
         if ( success )
            ::wxRemoveFile(mFilename.GetFullPath());
         else {
            // Restore original, if needed
            ::wxRemoveFile(mActualName.GetFullPath());
            ::wxRenameFile(mFilename.GetFullPath(), mActualName.GetFullPath());
         }
         // Restore filename
         mFilename = mActualName;
      }
      else {
         if ( ! success )
            // Remove any new, and only partially written, file.
            ::wxRemoveFile(mFilename.GetFullPath());
      }
   } );

   std::unique_ptr<ProgressDialog> pDialog;
   auto result = mPlugins[mFormat]->Export(mProject,
                                       pDialog,
                                       mChannels,
                                       mActualName.GetFullPath(),
                                       mSelectedOnly,
                                       mT0,
                                       mT1,
                                       mMixerSpec.get(),
                                       NULL,
                                       mSubFormat);

   success =
      result == ProgressResult::Success || result == ProgressResult::Stopped;

   return success;
}

void Exporter::CreateUserPaneCallback(wxWindow *parent, wxUIntPtr userdata)
{
   Exporter *self = (Exporter *) userdata;
   if (self)
   {
      self->CreateUserPane(parent);
   }
}

void Exporter::CreateUserPane(wxWindow *parent)
{
   ShuttleGui S(parent, eIsCreating);

   S.StartStatic(XO("Format Options"), 1);
   {
      S.StartHorizontalLay(wxEXPAND);
      {
         mBook = S.Position(wxEXPAND).StartSimplebook();
         {
            for (const auto &pPlugin : mPlugins)
            {
               for (int j = 0; j < pPlugin->GetFormatCount(); j++)
               {
                  // Name of simple book page is not displayed
                  S.StartNotebookPage( {} );
                  {
                     pPlugin->OptionsCreate(S, j);
                  }
                  S.EndNotebookPage();
               }
            }
         }
         S.EndSimplebook();

         auto b = safenew wxBitmapButton(S.GetParent(), wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));
         b->SetToolTip( XO("Help").Translation() );
         b->SetLabel(XO("Help").Translation());       // for screen readers
         S.Position(wxALIGN_BOTTOM | wxRIGHT | wxBOTTOM).AddWindow(b);
      }
      S.EndHorizontalLay();
   }
   S.EndStatic();

   return;
}

void Exporter::OnFilterChanged(wxFileCtrlEvent & evt)
{
   int index = evt.GetFilterIndex();

   // On GTK, this event can fire before the userpane is created
   if (mBook == NULL || index < 0 || index >= (int) mBook->GetPageCount())
   {
      return;
   }

#if defined(__WXGTK__)
   // On Windows and MacOS, changing the filter in the dialog
   // automatically changes the extension of the current file
   // name. GTK doesn't, so do it here.
   {
      FileNames::FileTypes fileTypes;

      int i = -1;
      for (const auto &pPlugin : mPlugins)
      {
         ++i;
         for (int j = 0; j < pPlugin->GetFormatCount(); j++)
         {
            auto mask = pPlugin->GetMask(j);
            fileTypes.insert( fileTypes.end(), mask.begin(), mask.end() );
         }
      }
   }
#endif

   mBook->ChangeSelection(index);
}

bool Exporter::ProcessFromTimerRecording(bool selectedOnly,
                                         double t0,
                                         double t1,
                                         wxFileName fnFile,
                                         int iFormat,
                                         int iSubFormat,
                                         int iFilterIndex)
{
   // Save parms
   mSelectedOnly = selectedOnly;
   mT0 = t0;
   mT1 = t1;

   // Auto Export Parameters
   mFilename = fnFile;
   mFormat = iFormat;
   mSubFormat = iSubFormat;
   mFilterIndex = iFilterIndex;

   // Gather track information
   if (!ExamineTracks()) {
      return false;
   }

   // Check for down mixing
   if (!CheckMix(false)) {
      return false;
   }

   // Ensure filename doesn't interfere with project files.
   if (!CheckFilename()) {
      return false;
   }

   // Export the tracks
   bool success = ExportTracks();

   // Get rid of mixerspec
   mMixerSpec.reset();

   return success;
}

int Exporter::GetAutoExportFormat() {
   return mFormat;
}

int Exporter::GetAutoExportSubFormat() {
   return mSubFormat;
}

int Exporter::GetAutoExportFilterIndex() {
   return mFormat;
}

wxFileName Exporter::GetAutoExportFileName() {
   return mFilename;
}

bool Exporter::SetAutoExportOptions() {
   mFormat = -1;

   if( GetFilename()==false )
        return false;

   // Let user edit MetaData
   if (mPlugins[mFormat]->GetCanMetaData(mSubFormat)) {
      if (!DoEditMetadata( *mProject,
         XO("Edit Metadata Tags"),
         XO("Exported Tags"),
         ProjectSettings::Get(*mProject).GetShowId3Dialog())) {
         return false;
      }
   }

   return true;
}

//----------------------------------------------------------------------------
// ExportMixerPanel
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ExportMixerPanel, wxPanelWrapper)
    EVT_PAINT(ExportMixerPanel::OnPaint)
    EVT_MOUSE_EVENTS(ExportMixerPanel::OnMouseEvent)
END_EVENT_TABLE()

ExportMixerPanel::ExportMixerPanel( wxWindow *parent, wxWindowID id,
      MixerSpec *mixerSpec,
      wxArrayString trackNames,
      const wxPoint& pos, const wxSize& size):
   wxPanelWrapper(parent, id, pos, size)
   , mMixerSpec{mixerSpec}
   , mChannelRects{ mMixerSpec->GetMaxNumChannels() }
   , mTrackRects{ mMixerSpec->GetNumTracks() }
{
   mBitmap = NULL;
   mWidth = 0;
   mHeight = 0;
   mSelectedTrack = mSelectedChannel = -1;

   mTrackNames = trackNames;
}

ExportMixerPanel::~ExportMixerPanel()
{
}

//set the font on memDC such that text can fit in specified width and height
void ExportMixerPanel::SetFont(wxMemoryDC &memDC, const wxString &text, int width,
      int height )
{
   int l = 0, u = 13, m, w, h;
   wxFont font = memDC.GetFont();
   while( l < u - 1 )
   {
      m = ( l + u ) / 2;
      font.SetPointSize( m );
      memDC.SetFont( font );
      memDC.GetTextExtent( text, &w, &h );

      if( w < width && h < height )
         l = m;
      else
         u = m;
   }
   font.SetPointSize( l );
   memDC.SetFont( font );
}

void ExportMixerPanel::OnPaint(wxPaintEvent & /* event */)
{
   wxPaintDC dc( this );

   int width, height;
   GetSize( &width, &height );

   if( !mBitmap || mWidth != width || mHeight != height )
   {
      mWidth = width;
      mHeight = height;
      mBitmap = std::make_unique<wxBitmap>( mWidth, mHeight,24 );
   }

   wxColour bkgnd = GetBackgroundColour();
   wxBrush bkgndBrush( bkgnd, wxBRUSHSTYLE_SOLID );

   wxMemoryDC memDC;
   memDC.SelectObject( *mBitmap );

   //draw background
   wxRect bkgndRect;
   bkgndRect.x = 0;
   bkgndRect.y = 0;
   bkgndRect.width = mWidth;
   bkgndRect.height = mHeight;

   memDC.SetBrush( *wxWHITE_BRUSH );
   memDC.SetPen( *wxBLACK_PEN );
   memDC.DrawRectangle( bkgndRect );

   //box dimensions
   mBoxWidth = mWidth / 6;

   mTrackHeight = ( mHeight * 3 ) / ( mMixerSpec->GetNumTracks() * 4 );
   if( mTrackHeight > 30 )
      mTrackHeight = 30;

   mChannelHeight = ( mHeight * 3 ) / ( mMixerSpec->GetNumChannels() * 4 );
   if( mChannelHeight > 30 )
      mChannelHeight = 30;

   static double PI = 2 * acos( 0.0 );
   double angle = atan( ( 3.0 * mHeight ) / mWidth );
   double radius = mHeight / ( 2.0 * sin( PI - 2.0 * angle ) );
   double totAngle = ( asin( mHeight / ( 2.0 * radius ) ) * 2.0 );

   //draw tracks
   memDC.SetBrush( AColor::envelopeBrush );
   angle = totAngle / ( mMixerSpec->GetNumTracks() + 1 );

   int max = 0, w, h;
   for( unsigned int i = 1; i < mMixerSpec->GetNumTracks(); i++ )
      if( mTrackNames[ i ].length() > mTrackNames[ max ].length() )
         max = i;

   SetFont( memDC, mTrackNames[ max ], mBoxWidth, mTrackHeight );

   for( unsigned int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
   {
      mTrackRects[ i ].x = (int)( mBoxWidth * 2 + radius - radius *
         cos( totAngle / 2.0 - angle * ( i + 1 ) ) - mBoxWidth + 0.5 );
      mTrackRects[ i ].y = (int)( mHeight * 0.5 - radius *
            sin( totAngle * 0.5 - angle * ( i + 1.0 ) ) -
            0.5 * mTrackHeight + 0.5 );

      mTrackRects[ i ].width = mBoxWidth;
      mTrackRects[ i ].height = mTrackHeight;

      memDC.SetPen( mSelectedTrack == (int)i ? *wxRED_PEN : *wxBLACK_PEN );
      memDC.DrawRectangle( mTrackRects[ i ] );

      memDC.GetTextExtent( mTrackNames[ i ], &w, &h );
      memDC.DrawText( mTrackNames[ i ],
            mTrackRects[ i ].x + ( mBoxWidth - w ) / 2,
            mTrackRects[ i ].y + ( mTrackHeight - h ) / 2 );
   }

   //draw channels
   memDC.SetBrush( AColor::playRegionBrush[ 0 ] );
   angle = ( asin( mHeight / ( 2.0 * radius ) ) * 2.0 ) /
      ( mMixerSpec->GetNumChannels() + 1 );

   SetFont( memDC, wxT( "Channel: XX" ), mBoxWidth, mChannelHeight );
   memDC.GetTextExtent( wxT( "Channel: XX" ), &w, &h );

   for( unsigned int i = 0; i < mMixerSpec->GetNumChannels(); i++ )
   {
      mChannelRects[ i ].x = (int)( mBoxWidth * 4 - radius  + radius *
         cos( totAngle * 0.5 - angle * ( i + 1 ) ) + 0.5 );
      mChannelRects[ i ].y = (int)( mHeight * 0.5 - radius *
            sin( totAngle * 0.5 - angle * ( i + 1 ) ) -
            0.5 * mChannelHeight + 0.5 );

      mChannelRects[ i ].width = mBoxWidth;
      mChannelRects[ i ].height = mChannelHeight;

      memDC.SetPen( mSelectedChannel == (int)i ? *wxRED_PEN : *wxBLACK_PEN );
      memDC.DrawRectangle( mChannelRects[ i ] );

      memDC.DrawText( wxString::Format( _( "Channel: %2d" ), i + 1 ),
            mChannelRects[ i ].x + ( mBoxWidth - w ) / 2,
            mChannelRects[ i ].y + ( mChannelHeight - h ) / 2 );
   }

   //draw links
   memDC.SetPen( wxPen( *wxBLACK, mHeight / 200 ) );
   for( unsigned int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
      for( unsigned int j = 0; j < mMixerSpec->GetNumChannels(); j++ )
         if( mMixerSpec->mMap[ i ][ j ] )
            AColor::Line(memDC, mTrackRects[ i ].x + mBoxWidth,
                  mTrackRects[ i ].y + mTrackHeight / 2, mChannelRects[ j ].x,
                  mChannelRects[ j ].y + mChannelHeight / 2 );

   dc.Blit( 0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, FALSE );
}

double ExportMixerPanel::Distance( wxPoint &a, wxPoint &b )
{
   return sqrt( pow( a.x - b.x, 2.0 ) + pow( a.y - b.y, 2.0 ) );
}

//checks if p is on the line connecting la, lb with tolerance
bool ExportMixerPanel::IsOnLine( wxPoint p, wxPoint la, wxPoint lb )
{
   return Distance( p, la ) + Distance( p, lb ) - Distance( la, lb ) < 0.1;
}

void ExportMixerPanel::OnMouseEvent(wxMouseEvent & event)
{
   if( event.ButtonDown() )
   {
      bool reset = true;
      //check tracks
      for( unsigned int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
         if( mTrackRects[ i ].Contains( event.m_x, event.m_y ) )
         {
            reset = false;
            if( mSelectedTrack == (int)i )
               mSelectedTrack = -1;
            else
            {
               mSelectedTrack = i;
               if( mSelectedChannel != -1 )
                  mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                     !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
            }
            goto found;
         }

      //check channels
      for( unsigned int i = 0; i < mMixerSpec->GetNumChannels(); i++ )
         if( mChannelRects[ i ].Contains( event.m_x, event.m_y ) )
         {
            reset = false;
            if( mSelectedChannel == (int)i )
               mSelectedChannel = -1;
            else
            {
               mSelectedChannel = i;
               if( mSelectedTrack != -1 )
                  mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ] =
                     !mMixerSpec->mMap[ mSelectedTrack ][ mSelectedChannel ];
            }
            goto found;
         }

      //check links
      for( unsigned int i = 0; i < mMixerSpec->GetNumTracks(); i++ )
         for( unsigned int j = 0; j < mMixerSpec->GetNumChannels(); j++ )
            if( mMixerSpec->mMap[ i ][ j ]  && IsOnLine( wxPoint( event.m_x,
                        event.m_y ), wxPoint( mTrackRects[ i ].x + mBoxWidth,
                           mTrackRects[ i ].y + mTrackHeight / 2 ),
                     wxPoint( mChannelRects[ j ].x, mChannelRects[ j ].y +
                     mChannelHeight / 2 ) ) )
               mMixerSpec->mMap[ i ][ j ] = false;

found:
      if( reset )
         mSelectedTrack = mSelectedChannel = -1;
      Refresh( false );
   }
}

//----------------------------------------------------------------------------
// ExportMixerDialog
//----------------------------------------------------------------------------

enum
{
   ID_MIXERPANEL = 10001,
   ID_SLIDER_CHANNEL
};

ExportMixerDialog::ExportMixerDialog( const TrackList *tracks, bool selectedOnly,
      unsigned maxNumChannels, wxWindow *parent, wxWindowID id, const TranslatableString &title,
      const wxPoint &position, const wxSize& size, long style ) :
   wxDialogWrapper( parent, id, title, position, size, style | wxRESIZE_BORDER )
{
   Bind(wxEVT_BUTTON, &ExportMixerDialog::OnOk, this, wxID_OK);
   Bind(wxEVT_BUTTON, &ExportMixerDialog::OnCancel, this, wxID_CANCEL);
   Bind(wxEVT_BUTTON, &ExportMixerDialog::OnMixerPanelHelp, this, wxID_HELP);
   Bind(wxEVT_SIZE, &ExportMixerDialog::OnSize, this);
   Bind(wxEVT_SLIDER, &ExportMixerDialog::OnSlider, this, ID_SLIDER_CHANNEL);

   SetName();

   unsigned numTracks = 0;

   bool anySolo = !(( tracks->Any<const WaveTrack>() + &WaveTrack::GetSolo ).empty());

   for (auto t :
         tracks->Any< const WaveTrack >()
            + ( selectedOnly ? &Track::IsSelected : &Track::Any  )
            - ( anySolo ? &WaveTrack::GetNotSolo :  &WaveTrack::GetMute)
   ) {
      numTracks++;
      const wxString sTrackName = (t->GetName()).Left(20);
      if( t->GetChannel() == Track::LeftChannel )
      /* i18n-hint: track name and L abbreviating Left channel */
         mTrackNames.push_back( wxString::Format( _( "%s - L" ), sTrackName ) );
      else if( t->GetChannel() == Track::RightChannel )
      /* i18n-hint: track name and R abbreviating Right channel */
         mTrackNames.push_back( wxString::Format( _( "%s - R" ), sTrackName ) );
      else
         mTrackNames.push_back(sTrackName);
   }

   // JKC: This is an attempt to fix a 'watching brief' issue, where the slider is
   // sometimes not slidable.  My suspicion is that a mixer may incorrectly
   // state the number of channels - so we assume there are always at least two.
   // The downside is that if someone is exporting to a mono device, the dialog
   // will allow them to output to two channels. Hmm.  We may need to revisit this.

   if (maxNumChannels < 2 )
      // STF (April 2016): AMR (narrowband) and MP3 may export 1 channel.
      // maxNumChannels = 2;
      maxNumChannels = 1;
   if (maxNumChannels > 32)
      maxNumChannels = 32;

   mMixerSpec = std::make_unique<MixerSpec>(numTracks, maxNumChannels);

   auto label = XO("Output Channels: %2d")
      .Format( mMixerSpec->GetNumChannels() );

   ShuttleGui S{ this, eIsCreating };
   {
      S.SetBorder( 5 );

      auto mixerPanel = safenew ExportMixerPanel(
         S.GetParent(), ID_MIXERPANEL, mMixerSpec.get(),
         mTrackNames, wxDefaultPosition, wxSize(400, -1));
      S.Prop(1)
         .Name(XO("Mixer Panel"))
         .Position(wxEXPAND | wxALL)
         .AddWindow(mixerPanel);

      S.StartHorizontalLay(wxALIGN_CENTER | wxALL, 0);
      {
         mChannelsText = S.AddVariableText(
            label,
            false, wxALIGN_LEFT | wxALL );

         S
            .Id(ID_SLIDER_CHANNEL)
            .Name(label)
            .Size({300, -1})
            .Style(wxSL_HORIZONTAL)
            .Position(wxEXPAND | wxALL)
            .AddSlider( {},
               mMixerSpec->GetNumChannels(),
               mMixerSpec->GetMaxNumChannels(), 1 );
      }
      S.EndHorizontalLay();

      S.AddStandardButtons( eCancelButton | eOkButton | eHelpButton );
   }

   SetAutoLayout(true);
   GetSizer()->Fit( this );
   GetSizer()->SetSizeHints( this );

   SetSizeHints( 640, 480, 20000, 20000 );

   SetSize( 640, 480 );
   Center();
}

ExportMixerDialog::~ExportMixerDialog()
{
}

void ExportMixerDialog::OnSize(wxSizeEvent &event)
{
   ExportMixerPanel *pnl = ( ( ExportMixerPanel* ) FindWindow( ID_MIXERPANEL ) );
   pnl->Refresh( false );
   event.Skip();
}

void ExportMixerDialog::OnSlider( wxCommandEvent & /* event */)
{
   wxSlider *channels = ( wxSlider* )FindWindow( ID_SLIDER_CHANNEL );
   ExportMixerPanel *pnl = ( ( ExportMixerPanel* ) FindWindow( ID_MIXERPANEL ) );
   mMixerSpec->SetNumChannels( channels->GetValue() );
   pnl->Refresh( false );
   wxString label;
   label.Printf( _( "Output Channels: %2d" ), mMixerSpec->GetNumChannels() );
   mChannelsText->SetLabel( label );
   channels->SetName( label );
}

void ExportMixerDialog::OnOk(wxCommandEvent & /* event */)
{
   EndModal( wxID_OK );
}

void ExportMixerDialog::OnCancel(wxCommandEvent & /* event */)
{
   EndModal( wxID_CANCEL );
}

void ExportMixerDialog::OnMixerPanelHelp(wxCommandEvent & /* event */)
{
   HelpSystem::ShowHelp(this, L"Advanced_Mixing_Options", true);
}


TranslatableString AudacityExportCaptionStr()
{
   return XO("Warning");
}
TranslatableString AudacityExportMessageStr()
{
   return XO("Unable to export.\nError %s");
}


// This creates a generic export error dialog
// Untranslated ErrorCodes like "MP3:1882" are used since we don't yet have
// a good user facing error message.  They allow us to 
// distinguish where the error occurred, and we can update the landing
// page as we learn more about when (if ever) these errors actually happen.
// The number happens to at one time have been a line number, but all
// we need from them is that they be distinct.
void ShowExportErrorDialog(wxString ErrorCode,
   TranslatableString message,
   const TranslatableString& caption)
{
   using namespace BasicUI;
   ShowErrorDialog( {},
      caption,
      message.Format( ErrorCode ),
      "Editing_Part_2#your-disk-is-full-or-not-writable", // URL.
      ErrorDialogOptions{ ErrorDialogType::ModalErrorReport } );
}

void ShowDiskFullExportErrorDialog(const wxFileNameWrapper &fileName)
{
   BasicUI::ShowErrorDialog( {},
      XO("Warning"),
      FileException::WriteFailureMessage(fileName),
      "Editing_Part_2#your-disk-is-full-or-not-writable"
   );
}


