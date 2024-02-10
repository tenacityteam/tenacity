/**********************************************************************

  Tenacity

  FFmpegPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class FFmpegPrefs
\brief A PrefsPanel that is only used to manage FFmpeg libraries;
historically, this PrefsPanel was also used to manage LAME in
Audacity as well.

*//*******************************************************************/


#include "FFmpegPrefs.h"

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


FFmpegPrefs::FFmpegPrefs(wxWindow * parent, wxWindowID winid)
/* i18-hint: refers to optional plug-in software libraries */
:   PrefsPanel(parent, winid, XO("FFmpeg"))
{
   // FFmpeg download button handler
   Bind(
      wxEVT_BUTTON,
      [this](wxCommandEvent&) {
         HelpSystem::ShowHelp(this, L"FFmpeg", true);
      },
      ID_FFMPEG_DOWN_BUTTON
   );

   Populate();
}

FFmpegPrefs::~FFmpegPrefs()
{
}

ComponentInterfaceSymbol FFmpegPrefs::GetSymbol()
{
   return FFMPEG_PREFS_PLUGIN_SYMBOL;
}

TranslatableString FFmpegPrefs::GetDescription()
{
   return XO("Preferences for FFmpeg");
}

ManualPageID FFmpegPrefs::HelpPageName()
{
   return "Preferences#ffmpeg";
}

/// Creates the dialog and its contents.
void FFmpegPrefs::Populate()
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
void FFmpegPrefs::PopulateOrExchange(ShuttleGui & S)
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

void FFmpegPrefs::SetFFmpegVersionText()
{
   mFFmpegVersion->SetValue(GetFFmpegVersion());
}

void FFmpegPrefs::OnFFmpegFindButton(wxCommandEvent & WXUNUSED(event))
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

bool FFmpegPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

#if !defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
namespace{
PrefsPanel::Registration sAttachment{ "FFmpeg",
   [](wxWindow *parent, wxWindowID winid, TenacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew FFmpegPrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::Before, "Directories" } }
};
}
#endif
