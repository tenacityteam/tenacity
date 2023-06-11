/**********************************************************************

  Audacity: A Digital Audio Editor

  LibraryPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class LibraryPrefs
\brief A PrefsPanel that is only used to manage FFmpeg libraries;
historically, this PrefsPanel was also used to manage LAME in
Audacity as well.

*//*******************************************************************/


#include "LibraryPrefs.h"

#include <wx/defs.h>
#include <wx/button.h>
#include <wx/stattext.h>

#include "../ffmpeg/FFmpeg.h"
#include "../widgets/HelpSystem.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/ReadOnlyText.h"
#include "../widgets/wxTextCtrlWrapper.h"


////////////////////////////////////////////////////////////////////////////////

constexpr int ID_FFMPEG_FIND_BUTTON = 7003;
constexpr int ID_FFMPEG_DOWN_BUTTON = 7004;

BEGIN_EVENT_TABLE(LibraryPrefs, PrefsPanel)
   EVT_BUTTON(ID_FFMPEG_FIND_BUTTON, LibraryPrefs::OnFFmpegFindButton)
   EVT_BUTTON(ID_FFMPEG_DOWN_BUTTON, LibraryPrefs::OnFFmpegDownButton)
END_EVENT_TABLE()

LibraryPrefs::LibraryPrefs(wxWindow * parent, wxWindowID winid)
/* i18-hint: refers to optional plug-in software libraries */
:   PrefsPanel(parent, winid, XO("Libraries"))
{
   Populate();
}

LibraryPrefs::~LibraryPrefs()
{
}

ComponentInterfaceSymbol LibraryPrefs::GetSymbol()
{
   return LIBRARY_PREFS_PLUGIN_SYMBOL;
}

TranslatableString LibraryPrefs::GetDescription()
{
   return XO("Preferences for Library");
}

ManualPageID LibraryPrefs::HelpPageName()
{
   return "Libraries_Preferences";
}

/// Creates the dialog and its contents.
void LibraryPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------

   SetFFmpegVersionText();
}

/// This PopulateOrExchange function is a good example of mixing the fully
/// automatic style of reading/writing from GUI to prefs with the partial form.
///
/// You'll notice that some of the Tie functions have Prefs identifiers in them
/// and others don't.
void LibraryPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("FFmpeg Import/Export Library"));
   {
      S.StartTwoColumn();
      {
         auto version =
#if defined(USE_FFMPEG)
            XO("No compatible FFmpeg library was found");
#else
            XO("FFmpeg support is not compiled in");
#endif

         mFFmpegVersion = S
            .Position(wxALIGN_CENTRE_VERTICAL)
            .AddReadOnlyText(XO("FFmpeg Library Version:"), version.Translation());

         S.AddVariableText(XO("FFmpeg Library:"),
            true, wxALL | wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);
         S.Id(ID_FFMPEG_FIND_BUTTON);
         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .AddButton(XXO("Loca&te..."),
                       wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
         S.AddVariableText(XO("FFmpeg Library:"),
            true, wxALL | wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);
         S.Id(ID_FFMPEG_DOWN_BUTTON);
         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .AddButton(XXO("Dow&nload"),
                       wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
      }
      S.EndTwoColumn();
   }
   S.EndStatic();
   S.EndScroller();

}

void LibraryPrefs::SetFFmpegVersionText()
{
   mFFmpegVersion->SetValue(GetFFmpegVersion());
}

void LibraryPrefs::OnFFmpegFindButton(wxCommandEvent & WXUNUSED(event))
{
#ifdef USE_FFMPEG
   bool showerrs =
#if defined(_DEBUG)
      true;
#else
      false;
#endif
   // Load the libs ('true' means that all errors will be shown)
   bool locate = !LoadFFmpeg(showerrs);

   // Libs are fine, don't show "locate" dialog unless user really wants it
   if (!locate) {
      int response = AudacityMessageBox(
         XO(
"Tenacity has automatically detected valid FFmpeg libraries.\nDo you still want to locate them manually?"),
         XO("Success"),
         wxCENTRE | wxYES_NO | wxNO_DEFAULT |wxICON_QUESTION);
      if (response == wxYES) {
        locate = true;
      }
   }

   if (locate) {
      // Show "Locate FFmpeg" dialog
      FindFFmpegLibs(this);
      LoadFFmpeg(showerrs);
   }

   SetFFmpegVersionText();
#endif
}

void LibraryPrefs::OnFFmpegDownButton(wxCommandEvent & WXUNUSED(event))
{
   HelpSystem::ShowHelp(this, L"FFmpeg", true);
}

bool LibraryPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

#if !defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
namespace{
PrefsPanel::Registration sAttachment{ "Library",
   [](wxWindow *parent, wxWindowID winid, TenacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew LibraryPrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::Before, "Directories" } }
};
}
#endif
