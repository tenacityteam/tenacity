/*  SPDX-License-Identifier: GPL-2.0-or-later */
/*!********************************************************************

  Tenacity

  WelcomeDialog.cpp

  Vitaly Sverchinsky
  Avery King

**********************************************************************/

#include "WelcomeDialog.h"

#include <wx/fs_mem.h>
#include <wx/settings.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <wx/hyperlink.h>
#include <wx/checkbox.h>
#include <wx/frame.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include "HelpSystem.h"
#include "HelpText.h"
#include "ProjectWindows.h"
#include "ShuttleGui.h"
#include "Theme.h"
#include "AllThemeResources.h"

#include "../images/TenacityLogoWithName.xpm"

namespace
{

constexpr auto ChangeLogURL = "https://codeberg.org/tenacityteam/tenacity/releases";

constexpr auto WindowWidth = 440;

// wxHTML renders text with smaller line spacing on macOS
#if defined (__WXOSX__)
constexpr auto WindowHeight = 470;
#else
constexpr auto WindowHeight = 490;
#endif

}
AttachedWindows::RegisteredFactory sWhatsNewWindow{
   []( AudacityProject &project ) -> wxWeakRef< wxWindow > {
      auto &window = GetProjectFrame(project);
      return safenew WelcomeDialog(&window, wxID_ANY);
   }
};


namespace
{

wxString MakeWhatsNewText()
{
   wxStringOutputStream o;
   wxTextOutputStream s(o);

   s
      << wxT("<center><h2>")
      << XO("Welcome to Tenacity!")
      << wxT("</h2></center><br>")

      /* i18n-hint: %s is the program's version string */
      #if defined(IS_ALPHA) || defined(IS_BETA)
      << wxT("<b>")
      << XO("This is a pre-release version of Tenacity. You may experience (unknown) bugs and, worst case, possible data loss. Proceed with caution.")
      << wxT("</b>")
      #else
      << XO("You are using Tenacity %s. Tenacity is a fork of Audacity").Format(TENACITY_VERSION_STRING)
      #endif

      << wxT("<center><h2>")
      << XO("Need Help?")
      << wxT("</h2></center>")
      << XO("We have several support methods:")
      << wxT("<p><ul><li>")
      /* i18n-hint: Preserve '[[help:Quick_Help|' as it's the name of a link.*/
      << XO("[[help:Quick_Help|Quick Help]]")
      << wxT("</li><li>")
      << XO(
/* i18n-hint: Preserve '[[help:Main_Page|' as it's the name of a link.*/
" [[help:Main_Page|Tenacity Manual]] - if not installed locally, [[https://tenacityaudio.org/docs/index.html|view online]]")
      << wxT("</li></ul></p><p>")
      << wxT("<b>")
      << XO("More:</b> Visit our [[https://codeberg.org/tenacityteam/tenacity/wiki|Wiki]] for tips, tricks, extra tutorials and effects plug-ins.")
      << wxT("</p>");

   auto result = o.GetString();

   return FormatHtmlText(o.GetString());
}

}

BEGIN_EVENT_TABLE(WelcomeDialog, wxDialogWrapper)
   EVT_BUTTON(wxID_OK, WelcomeDialog::OnOK)
END_EVENT_TABLE()

WelcomeDialog::WelcomeDialog(wxWindow* parent, wxWindowID id)
   : wxDialogWrapper(parent, id, XO("Welcome to Tenacity!"))
{
   SetSize(FromDIP(wxSize(WindowWidth, WindowHeight)));
   SetName();
   ShuttleGui S( this, eIsCreating );
   Populate( S );
   Centre();
}

WelcomeDialog::~WelcomeDialog() = default;

void WelcomeDialog::Show(AudacityProject& project)
{
   auto dialog = &GetAttachedWindows(project)
      .Get<WelcomeDialog>(sWhatsNewWindow);
   dialog->CenterOnParent();
   dialog->wxDialogWrapper::Show();
}

void WelcomeDialog::Populate(ShuttleGui& S)
{
   bool showSplashScreen;
   gPrefs->Read(wxT("/GUI/ShowSplashScreen"), &showSplashScreen, true );

   S.StartHorizontalLay(wxEXPAND);
   {
      S.StartVerticalLay(wxEXPAND | wxLEFT);
      {
         constexpr float fScale = 0.75f;// smaller size.
         wxImage RescaledImage( TenacityLogoWithName_xpm );
         wxColour MainColour( 
            RescaledImage.GetRed(1,1), 
            RescaledImage.GetGreen(1,1), 
            RescaledImage.GetBlue(1,1));

         RescaledImage.Rescale( (int)(LOGOWITHNAME_WIDTH * fScale), (int)(LOGOWITHNAME_HEIGHT *fScale), wxIMAGE_QUALITY_HIGH );
         wxBitmap RescaledBitmap( RescaledImage );
         wxStaticBitmap *const icon =
            safenew wxStaticBitmap(S.GetParent(), -1,
                              //*m_pLogo, //v theTheme.Bitmap(bmpAudacityLogoWithName),
                              RescaledBitmap,
                              wxDefaultPosition,
                              wxSize((int)(LOGOWITHNAME_WIDTH*fScale), (int)(LOGOWITHNAME_HEIGHT*fScale)));

         S.Prop(0).AddWindow( icon );

         const auto whatsnew = safenew LinkingHtmlWindow(S.GetParent());
         whatsnew->SetPage(MakeWhatsNewText());
         S
            .Prop(1)
            .Position(wxEXPAND | wxALL)
            .AddWindow(whatsnew);
      }
      S.EndVerticalLay();
   }
   S.EndHorizontalLay();
   S.AddSpace(25);
   const auto line = safenew wxWindow(S.GetParent(), wxID_ANY);
   line->SetSize(-1, 1);

   S
      .Prop(0)
      .Position(wxEXPAND)
      .AddWindow(line);

   S.StartHorizontalLay(wxALIGN_CENTRE, 0);
   {
      S.SetBorder(10);
      #if defined(IS_ALPHA) || defined(IS_BETA)
      const auto bugReportsLink = new wxHyperlinkCtrl(
         S.GetParent(),
         wxID_ANY,
         _("Report Bugs"),
         "https://codeberg.org/tenacityteam/tenacity/issues"
      );
      S
         .Position(wxTOP | wxBOTTOM)
         .AddWindow(bugReportsLink);

      S.AddSpace(25);
      #endif

      const auto tutorialsLink = safenew wxHyperlinkCtrl(
         S.GetParent(),
         wxID_ANY,
         _("View online manual"),
         "https://tenacityaudio.org/docs/index.html");
      S
         .Position(wxTOP | wxBOTTOM)
         .AddWindow(tutorialsLink);

      S.AddSpace(25);

      const auto forumLink = safenew wxHyperlinkCtrl(
         S.GetParent(),
         wxID_ANY,
         _("Visit our Matrix Room"),
         "https://matrix.to/#/#tenacity2:matrix.org");
      S
         .Position(wxTOP | wxBOTTOM)
         .AddWindow(forumLink);
   }
   S.EndHorizontalLay();

   S.Position(wxEXPAND).StartPanel(2);
   {
      S.StartHorizontalLay(wxEXPAND);
      {
         S.SetBorder(4);
         mDontShowAgain = S
            .Position(wxALL | wxALIGN_CENTRE)
            .AddCheckBox( XXO("Don't show this again at start up"), !showSplashScreen);
         
         S.AddSpace(1,1,1);

         S
            .Id(wxID_OK)
            .AddButton(XXO("OK"), wxALL, true);
      }
      S.EndHorizontalLay();
   }
   S.EndPanel();
}

void WelcomeDialog::OnOK(wxCommandEvent& evt)
{
   gPrefs->Write(wxT("/GUI/ShowSplashScreen"), !mDontShowAgain->IsChecked() );
   gPrefs->Flush();
   wxDialogWrapper::Show(false);
}
