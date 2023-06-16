/**********************************************************************

  Audacity: A Digital Audio Editor

  HelpSystem.cpp

  Jimmy Johnson
  Leland Lucius
  Richard Ash

  was merged with LinkingHtmlWindow.h

  Vaughan Johnson
  Dominic Mazzoni

  utility fn and
  descendant of HtmlWindow that opens links in the user's
  default browser

*//********************************************************************/


#include "HelpSystem.h"

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/button.h>
#include <wx/frame.h>
#include <wx/icon.h>
#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/html/htmlwin.h>
#include <wx/settings.h>
#include <wx/statusbr.h>
#include <wx/regex.h>

// Tenacity libraries
#include <lib-files/FileNames.h>
#include <lib-files/wxFileNameWrapper.h>
#include <lib-preferences/Prefs.h>

#include "../theme/AllThemeResources.h"
#include "../shuttle/ShuttleGui.h"
#include "../theme/Theme.h"
#include "../HelpText.h"
#include "../prefs/GUIPrefs.h"

#ifdef USE_ALPHA_MANUAL
const wxString HelpSystem::HelpHostname = wxT("tenacityaudio.org");
const wxString HelpSystem::HelpServerHomeDir = wxT("/docs/");
const wxString HelpSystem::HelpServerManDir = wxT("/docs/_content/");
#else
const wxString HelpSystem::HelpHostname = wxT("tenacityaudio.org");
const wxString HelpSystem::HelpServerHomeDir = wxT("/docs/");
const wxString HelpSystem::HelpServerManDir = wxT("/docs/_content/");
#endif
const wxString HelpSystem::LocalHelpManDir = wxT("/_content/");

/// Mostly we use this so that we have the code for resizability
/// in one place.  Other considerations like screen readers are also
/// handled by having the code in one place.
void HelpSystem::ShowInfoDialog( wxWindow *parent,
                     const TranslatableString &dlogTitle,
                     const TranslatableString &shortMsg,
                     const wxString &message,
                     const int xSize, const int ySize)
{
   wxDialogWrapper dlog(parent, wxID_ANY,
                dlogTitle,
                wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX /*| wxDEFAULT_FRAME_STYLE */);

   dlog.SetName();
   ShuttleGui S(&dlog, eIsCreating);

   S.StartVerticalLay(1);
   {
      S.AddTitle( shortMsg );
      S.Style( wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_RICH2 |
              wxTE_AUTO_URL | wxTE_NOHIDESEL | wxHSCROLL )
         .AddTextWindow(message);

      S.SetBorder( 0 );
      S.StartHorizontalLay(wxALIGN_CENTER_HORIZONTAL, 0);
         S.AddStandardButtons(eOkButton);
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   // Smallest size is half default size.  Seems reasonable.
   dlog.SetMinSize( wxSize(xSize/2, ySize/2) );
   dlog.SetSize( wxSize(xSize, ySize) );
   dlog.Center();
   dlog.ShowModal();
}

void HelpSystem::ShowHtmlText(wxWindow *pParent,
                  const TranslatableString &Title,
                  const wxString &HtmlText,
                  bool bModal)
{
   wxASSERT(pParent);

   HtmlTextDialog pWnd(pParent, Title);
   pWnd.SetHtml(HtmlText);

   // -- START of ICON stuff -----
   // If this section (providing an icon) causes compilation errors on linux, comment it out for now.
   // it will just mean that the icon is missing.  Works OK on Windows.
   #ifdef __WXMSW__
   wxIcon ic{ wxICON(AudacityLogo) };
   #else
   wxIcon ic{};
      ic.CopyFromBitmap(theTheme.Bitmap(bmpAudacityLogo48x48));
   #endif
   pWnd.SetIcon( ic );
   // -- END of ICON stuff -----

   if (bModal)
   {
      pWnd.ShowModal();
   } else
   {
      pWnd.Show(true);
   }
}

// Shows help in browser, or possibly in own dialog.
void HelpSystem::ShowHelp(wxWindow *parent,
                    const FilePath &localFileName,
                    const URLString &remoteURL,
                    bool bModal)
{
   wxASSERT(parent); // to justify safenew
   wxString HelpMode = wxT("Local");

   gPrefs->Read(wxT("/GUI/Help"), &HelpMode, wxT("Local") );

   // Anchors (URLs with a '#' in them) are not supported by many OSs for local file names
   // See, for example, https://groups.google.com/forum/#!topic/wx-users/pC0uOZJalRQ
   // Problems have been reported on Win, Mac and some versions of Linux.
   // So we set HelpMode to use the internet if an anchor is found.
   if (localFileName.Find('#', true) != wxNOT_FOUND)
      HelpMode = wxT("FromInternet");
   // Until a solution is found for this, the next few lines are irrelevant.

   // Obtain the local file system file name, without the anchor if present.
   wxString localfile;
   if (localFileName.Find('#', true) != wxNOT_FOUND)
      localfile = localFileName.BeforeLast('#');
   else
      localfile = localFileName;

   if( (HelpMode == wxT("FromInternet")) && !remoteURL.empty() )
   {
      // Always go to remote URL.  Use External browser.
      OpenInDefaultBrowser( remoteURL );
   }
   else if( localfile.empty() || !wxFileExists( localfile ))
   {
      // If you give an empty remote URL, you should have already ensured
      // that the file exists!
      wxASSERT( !remoteURL.empty() );
      // I can't find it'.
      // Use Built-in browser to suggest you use the remote url.
      wxString Text = HelpText( wxT("remotehelp") );
      Text.Replace( wxT("*URL*"), remoteURL.GET() );
      // Always make the 'help on the internet' dialog modal.
      // Fixes Bug 1411.
      ShowHtmlText( parent, XO("Help on the Internet"), Text, true );
   }
   else if( HelpMode == wxT("Local"))
   {
      // Local file, External browser
      OpenInDefaultBrowser( L"file:" + localFileName );
   }
}

void HelpSystem::ShowHelp(wxWindow *parent,
                          const ManualPageID &PageName,
                          bool bModal)
{
   /// The string which is appended to the development manual page name in order
   /// obtain the file name in the local and release web copies of the manual
   const wxString ReleaseSuffix = L".html";

   FilePath localHelpPage;
   wxString webHelpPath;
   wxString webHelpPage;
   wxString releasePageName;
   wxString anchor;	// optional part of URL after (and including) the '#'
   const auto &PageNameStr = PageName.GET();
   if (PageNameStr.Find('#', true) != wxNOT_FOUND)
   {	// need to split anchor off into separate variable
      releasePageName = PageNameStr.BeforeLast('#');
      anchor = wxT("#") + PageNameStr.AfterLast('#');
   }
   else
   {
      releasePageName = PageName.GET();
      anchor = wxT("");
   }

   // Tenacity manual page names are typically the page names themselves. For
   // example, the FFmpeg manual page is named 'FFmpeg'.
   //
   // Unlike with Audacity manual page names, our manual page names do not need
   // any transformation before showing the pages.
   //
   // There is one special case, however: Main_Page. The reason why this was
   // preserved was to help differ the main manual page differently from just
   // "index.html".
   if (releasePageName == L"Main_Page")
   {
      releasePageName = L"index" + ReleaseSuffix + anchor;
      localHelpPage = wxFileName(FileNames::HtmlHelpDir(), releasePageName).GetFullPath();
      webHelpPath = L"https://" + HelpSystem::HelpHostname + HelpSystem::HelpServerHomeDir;
   }
   else if (releasePageName == L"Quick_Help")
   {
      releasePageName = L"quick_help" + ReleaseSuffix + anchor;
      localHelpPage = wxFileName(FileNames::HtmlHelpDir(), releasePageName).GetFullPath();
      webHelpPath = L"https://" + HelpSystem::HelpHostname + HelpSystem::HelpServerHomeDir;
   }
   // not a page name, but rather a full path (e.g. to wiki)
   // in which case do not do any substitutions.
   else if (releasePageName.StartsWith( "http" ) )
   {
      localHelpPage = "";
      releasePageName += anchor;
      // webHelpPath remains empty
   }

   // GP: FIXME: page name transformation might not be necessary anymore.
   else
   {
      // Handle all other pages.
      wxRegEx re;
      // replace 'special characters' with underscores.
      // RFC 2396 defines the characters a-z, A-Z, 0-9 and ".-_" as "always safe"
      // mw2html also replaces "-" with "_" so replace that too.
      
      // If PageName contains a %xx code, mw2html will transform it:
      // '%xx' => '%25xx' => '_'
      re.Compile(wxT("%.."));
      re.ReplaceAll(&releasePageName, (wxT("_")));
      // Now replace all other 'not-safe' characters.
      re.Compile(wxT("[^[:alnum:] . [:space:]]"));
      re.ReplaceAll(&releasePageName, (wxT("_")));
      // Replace spaces with "+"
      releasePageName.Replace(wxT(" "), wxT("+"), true);
      // Reduce multiple underscores to single underscores
      re.Compile(wxT("__+"));
      re.ReplaceAll(&releasePageName, (wxT("_")));
      // Replace "_." with "."
      releasePageName.Replace(wxT("_."), wxT("."), true);
      // Concatenate file name with file extension and anchor.
      releasePageName = releasePageName + ReleaseSuffix + anchor;
      // Other than index and quick_help, all local pages are in subdirectory 'LocalHelpManDir'.
      localHelpPage = wxFileName(FileNames::HtmlHelpDir() + LocalHelpManDir, releasePageName).GetFullPath();
      // Other than index and quick_help, all on-line pages are in subdirectory 'HelpServerManDir'.
      webHelpPath = L"https://" + HelpSystem::HelpHostname + HelpSystem::HelpServerManDir;
   }

   webHelpPage = webHelpPath + releasePageName;

   wxLogMessage(wxT("Help button pressed: PageName %s, releasePageName %s"),
              PageName.GET(), releasePageName);
   wxLogMessage(wxT("webHelpPage %s, localHelpPage %s"),
              webHelpPage, localHelpPage);

   wxASSERT(parent); // to justify safenew

   HelpSystem::ShowHelp(
      parent, 
      localHelpPage,
      webHelpPage,
      bModal
      );
}

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/mimetype.h>
#include <wx/filename.h>
#include <wx/uri.h>

BEGIN_EVENT_TABLE(HtmlTextDialog, wxDialogWrapper)
   EVT_BUTTON(wxID_CANCEL,   HtmlTextDialog::OnClose)
   EVT_KEY_DOWN(HtmlTextDialog::OnKeyDown)
END_EVENT_TABLE()


HtmlTextDialog::HtmlTextDialog(wxWindow *pParent, const TranslatableString &title)
   : wxDialogWrapper{ pParent, ID, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER /*| wxMAXIMIZE_BOX */  }
{
   ShuttleGui S(this, eIsCreating);

   S.Style( wxNO_BORDER | wxTAB_TRAVERSAL )
      .Prop(true)
      .StartPanel();
   {
      mpHtml = new LinkingHtmlWindow(
         S.GetParent(),
         wxID_ANY,
         wxDefaultPosition,
         wxSize(480, 240),
         wxHW_SCROLLBAR_AUTO
      );

      S.Prop(1).Focus().Position( wxEXPAND );
      S.AddWindow(mpHtml);
      S.Id( wxID_CANCEL ).AddButton( XXO("Close"), wxALIGN_CENTER, true );
   }
   S.EndPanel();
}

HtmlTextDialog::~HtmlTextDialog()
{
}

void HtmlTextDialog::SetHtml(const wxString& text)
{
   mpHtml->SetPage(text);
}

void HtmlTextDialog::OnClose(wxCommandEvent & WXUNUSED(event))
{
   if (IsModal() && !mDismissed)
   {
      mDismissed = true;
      EndModal(wxID_CANCEL);
   }
}

void HtmlTextDialog::OnKeyDown(wxKeyEvent & event)
{
   bool bSkip = true;
   if (event.GetKeyCode() == WXK_ESCAPE)
   {
      bSkip = false;
      Close(false);
   }
   event.Skip(bSkip);
}

void OpenInDefaultBrowser(const URLString& link)
{
   wxURI uri(link.GET());
   wxLaunchDefaultBrowser(uri.BuildURI());
}

LinkingHtmlWindow::LinkingHtmlWindow(wxWindow *parent, wxWindowID id /*= -1*/,
                                       const wxPoint& pos /*= wxDefaultPosition*/,
                                       const wxSize& size /*= wxDefaultSize*/,
                                       long style /*= wxHW_SCROLLBAR_AUTO*/) :
   HtmlWindow(parent, id, pos, size, style)
{
}

void LinkingHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
   wxString href = link.GetHref();

   if( href.StartsWith( wxT("innerlink:help:")))
   {
      HelpSystem::ShowHelp(this, ManualPageID{ href.Mid( 15 ) }, true );
      return;
   }
   else if( href.StartsWith(wxT("innerlink:")) )
   {
      wxString FileName =
         wxFileName( FileNames::HtmlHelpDir(), href.Mid( 10 ) + wxT(".htm") ).GetFullPath();
      if( wxFileExists( FileName ) )
      {
         HelpSystem::ShowHelp(this, FileName, wxEmptyString, false);
         return;
      }
      else
      {
         SetPage( HelpText( href.Mid( 10 )));
         wxGetTopLevelParent(this)->SetLabel( TitleText( href.Mid( 10 )).Translation() );
      }
   }
   else if( href.StartsWith(wxT("mailto:")) || href.StartsWith(wxT("file:")) )
   {
      OpenInDefaultBrowser( link.GetHref() );
      return;
   }
   else if( !href.StartsWith( wxT("http:"))  && !href.StartsWith( wxT("https:")) )
   {
      HtmlWindow::OnLinkClicked( link );
   }
   else
   {
      OpenInDefaultBrowser(link.GetHref());
      return;
   }
   wxFrame * pFrame = GetRelatedFrame();
   if( !pFrame )
      return;
   wxWindow * pWnd = pFrame->FindWindow(HtmlTextDialog::ID);
   if( !pWnd )
      return;
   HtmlTextDialog * pDlg = wxDynamicCast( pWnd , HtmlTextDialog );
   if( !pDlg )
      return;
}
