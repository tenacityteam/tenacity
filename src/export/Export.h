/**********************************************************************

  Audacity: A Digital Audio Editor

  Export.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EXPORT__
#define __AUDACITY_EXPORT__

#include <functional>
#include <vector>
#include <wx/filename.h> // member variable

// Tenacity libraries
#include <lib-files/FileNames.h>
#include <lib-math/SampleFormat.h>
#include <lib-registries/Registry.h>
#include <lib-strings/Identifier.h>

#include "ExportPlugin.h"
#include "../widgets/wxPanelWrapper.h" // to inherit

class wxArrayString;
class FileDialogWrapper;
class wxFileCtrlEvent;
class wxMemoryDC;
class wxSimplebook;
class wxStaticText;
class TenacityProject;
class WaveTrack;
class Tags;
class TrackList;
class MixerSpec;
class Mixer;
using WaveTrackConstArray = std::vector < std::shared_ptr < const WaveTrack > >;
class wxFileNameWrapper;

//----------------------------------------------------------------------------
// Exporter
//----------------------------------------------------------------------------

// For a file suffix change from the options.
wxDECLARE_EXPORTED_EVENT(TENACITY_DLL_API,
   AUDACITY_FILE_SUFFIX_EVENT, wxCommandEvent);

class  TENACITY_DLL_API Exporter final : public wxEvtHandler
{
public:

   using ExportPluginFactory =
      std::function< std::unique_ptr< ExportPlugin >() >;

   // Objects of this type are statically constructed in files implementing
   // subclasses of ExportPlugin
   // Register factories, not plugin objects themselves, which allows them
   // to have some fresh state variables each time export begins again
   // and to compute translated strings for the current locale
   struct TENACITY_DLL_API RegisteredExportPlugin{
      RegisteredExportPlugin(
         const Identifier &id, // an internal string naming the plug-in
         const ExportPluginFactory&,
         const Registry::Placement &placement = { wxEmptyString, {} } );
   };

   static bool DoEditMetadata(TenacityProject &project,
      const TranslatableString &title,
      const TranslatableString &shortUndoDescription, bool force);

   Exporter( TenacityProject &project );
   virtual ~Exporter();

   void SetFileDialogTitle( const TranslatableString & DialogTitle );
   void SetDefaultFormat( const FileExtension & Format ){ mFormatName = Format;};

   bool Process(bool selectedOnly,
                double t0, double t1);
   bool Process(unsigned numChannels,
                const FileExtension &type, const wxString & filename,
                bool selectedOnly, double t0, double t1);

   void DisplayOptions(int index);
   int FindFormatIndex(int exportindex);

   const ExportPluginArray &GetPlugins();

   // Auto Export from Timer Recording
   bool ProcessFromTimerRecording(bool selectedOnly,
                                  double t0,
                                  double t1,
                                  wxFileName fnFile,
                                  int iFormat,
                                  int iSubFormat,
                                  int iFilterIndex);
   bool SetAutoExportOptions();
   int GetAutoExportFormat();
   int GetAutoExportSubFormat();
   int GetAutoExportFilterIndex();
   wxFileName GetAutoExportFileName();
   void OnHelp(wxCommandEvent &evt);

private:
   bool ExamineTracks();
   bool GetFilename();
   bool CheckFilename();
   bool CheckMix(bool prompt = true);
   bool ExportTracks();

   static void CreateUserPaneCallback(wxWindow *parent, wxUIntPtr userdata);
   void CreateUserPane(wxWindow *parent);
   void OnFilterChanged(wxFileCtrlEvent & evt);

private:
   FileExtension mFormatName;
   FileDialogWrapper *mDialog;
   TranslatableString mFileDialogTitle;
   TenacityProject *mProject;
   std::unique_ptr<MixerSpec> mMixerSpec;

   ExportPluginArray mPlugins;

   wxFileName mFilename;
   wxFileName mActualName;

   double mT0;
   double mT1;
   int mFilterIndex;
   int mFormat;
   int mSubFormat;
   int mNumSelected;
   unsigned mNumLeft;
   unsigned mNumRight;
   unsigned mNumMono;
   unsigned mChannels;
   bool mSelectedOnly;

   wxSimplebook *mBook;

   DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
// ExportMixerPanel
//----------------------------------------------------------------------------
class ExportMixerPanel final : public wxPanelWrapper
{
public:
   ExportMixerPanel( wxWindow *parent, wxWindowID id,
         MixerSpec *mixerSpec, wxArrayString trackNames,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize);
   virtual ~ExportMixerPanel();

   void OnMouseEvent(wxMouseEvent & event);
   void OnPaint(wxPaintEvent & event);

private:
   std::unique_ptr<wxBitmap> mBitmap;
   wxRect mEnvRect;
   int mWidth;
   int mHeight;
   MixerSpec *mMixerSpec;
   ArrayOf<wxRect> mChannelRects;
   ArrayOf<wxRect> mTrackRects;
   int mSelectedTrack, mSelectedChannel;
   wxArrayString mTrackNames;
   int mBoxWidth, mChannelHeight, mTrackHeight;

   void SetFont( wxMemoryDC &memDC, const wxString &text, int width, int height );
   double Distance( wxPoint &a, wxPoint &b );
   bool IsOnLine( wxPoint p, wxPoint la, wxPoint lb );

   DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
// ExportMixerDialog
//----------------------------------------------------------------------------
class ExportMixerDialog final : public wxDialogWrapper
{
public:
   // constructors and destructors
   ExportMixerDialog( const TrackList * tracks, bool selectedOnly, unsigned maxNumChannels,
         wxWindow *parent, wxWindowID id, const TranslatableString &title,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
   virtual ~ExportMixerDialog();

   MixerSpec* GetMixerSpec() { return mMixerSpec.get(); }

private:
   wxStaticText *mChannelsText;
   std::unique_ptr<MixerSpec> mMixerSpec;
   wxArrayString mTrackNames;

private:
   void OnOk( wxCommandEvent &event );
   void OnCancel( wxCommandEvent &event );
   void OnMixerPanelHelp( wxCommandEvent &event );
   void OnSlider( wxCommandEvent &event );
   void OnSize( wxSizeEvent &event );

private:
   DECLARE_EVENT_TABLE()
};

TENACITY_DLL_API TranslatableString AudacityExportCaptionStr();
TENACITY_DLL_API TranslatableString AudacityExportMessageStr();

/// We have many Export errors that are essentially anonymous
/// and are distinguished only by an error code number.
/// Rather than repeat the code, we have it just once.
TENACITY_DLL_API void ShowExportErrorDialog(wxString ErrorCode,
   TranslatableString message = AudacityExportMessageStr(),
   const TranslatableString& caption = AudacityExportCaptionStr());

TENACITY_DLL_API
void ShowDiskFullExportErrorDialog(const wxFileNameWrapper &fileName);

#endif
