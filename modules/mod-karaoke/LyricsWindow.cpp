/**********************************************************************

  Audacity: A Digital Audio Editor

  LyricsWindow.cpp

  Vaughan Johnson
  Dominic Mazzoni

**********************************************************************/

#include "LyricsWindow.h"
#include "Lyrics.h"
#include "AudioIO.h"
#include "CommonCommandFlags.h"
#include "LabelTrack.h"
#include "prefs/GUISettings.h" // for RTL_WORKAROUND
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectFileIO.h"
#include "ProjectWindow.h"
#include "ProjectWindows.h"
#include "ViewInfo.h"

#include <wx/app.h>
#include <wx/radiobut.h>
#include <wx/toolbar.h>
#include <wx/settings.h>

#ifdef __WXMSW__
#include "../images/TenacityLogo.h"
#else
#include "../images/TenacityLogo48x48.h"
#endif

#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif

#define AudacityKaraokeTitle XO("Tenacity Karaoke%s")

enum
{
   kID_RadioButton_BouncingBall = 10101,
   kID_RadioButton_Highlight,
};

const wxSize gSize = wxSize(LYRICS_DEFAULT_WIDTH, LYRICS_DEFAULT_HEIGHT);

LyricsWindow::LyricsWindow(TenacityProject *parent)
    : wxFrame(&GetProjectFrame(*parent), -1, wxString{},
              wxPoint(100, 300), gSize,
              // v Bug in wxFRAME_FLOAT_ON_PARENT:
              //  If both the project frame and LyricsWindow are minimized and you restore LyricsWindow,
              //  you can't restore project frame until you close LyricsWindow, but then project frame and
              //  LyricsWindow are restored but LyricsWindow is unresponsive because it thinks it's not shown.
              //     wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
              wxDEFAULT_FRAME_STYLE)
{
   Bind(wxEVT_CLOSE_WINDOW, &LyricsWindow::OnCloseWindow, this);
   Bind(wxEVT_RADIOBUTTON, &LyricsWindow::OnStyle_BouncingBall, this, kID_RadioButton_BouncingBall);
   Bind(wxEVT_RADIOBUTTON, &LyricsWindow::OnStyle_Highlight, this, kID_RadioButton_Highlight);

   // vvv Still necessary? It's commented out in ToolManager and Meter, so I did so here.
   //    #ifdef __WXMAC__
   //       // WXMAC doesn't support wxFRAME_FLOAT_ON_PARENT, so we do
   //       SetWindowClass((WindowRef) MacGetWindowRef(), kFloatingWindowClass);
   //    #endif
   auto pProject = parent->shared_from_this();
   mProject = pProject;

   SetWindowTitle();
   auto titleChanged = [&](wxCommandEvent &evt)
   {
      SetWindowTitle();
      evt.Skip();
   };
   wxTheApp->Bind(EVT_PROJECT_TITLE_CHANGE, titleChanged);

   // loads either the XPM or the windows resource, depending on the platform
#if !defined(__WXMAC__) && !defined(__WXX11__)
   {
#ifdef __WXMSW__
      wxIcon ic{wxICON(TenacityLogo)};
#else
      wxIcon ic{wxICON(TenacityLogo48x48)};
#endif
      SetIcon(ic);
   }
#endif

   wxPoint panelPos(0, 0);
   wxSize panelSize = gSize;

   // vvv not yet working right in ported version, so choice is disabled.
   //  It seems when you select highlight style, the TrackPanel timer stops working, but
   //  going back to bouncing ball style starts it up again (!!!), per breakpoints in TrackPanel::OnTimer().
   //
   // wxToolBar* pToolBar = this->CreateToolBar();
   // const int kHorizMargin = 8;
   // wxASSERT(pToolBar); // To justify safenew
   // wxRadioButton* pRadioButton_BouncingBall =
   //    safenew wxRadioButton(pToolBar, kID_RadioButton_BouncingBall, _("Bouncing Ball"), wxPoint(kHorizMargin, 4),
   //        wxDefaultSize, wxRB_GROUP);
   //// Reposition to center vertically.
   // wxSize tbSize = pToolBar->GetSize();
   // wxSize btnSize = pRadioButton_BouncingBall->GetSize();
   // int top = (tbSize.GetHeight() - btnSize.GetHeight()) / 2;
   // pRadioButton_BouncingBall->Move(kHorizMargin, top);
   // pToolBar->AddControl(pRadioButton_BouncingBall);
   //
   // int left = kHorizMargin + btnSize.GetWidth() + kHorizMargin; //vvv Doesn't actually work. Probably need sizers.
   // wxRadioButton* pRadioButton_Highlight =
   //    safenew wxRadioButton(pToolBar, kID_RadioButton_Highlight, _("Highlight"), wxPoint(left, top));
   // pToolBar->AddControl(pRadioButton_Highlight);
   //
   // panelPos.x += tbSize.GetHeight();
   // panelSize.y -= tbSize.GetHeight();
   //
   // #if defined(__WXMAC__)
   //    wxColour face = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
   //    pRadioButton_BouncingBall->SetBackgroundColour(face);
   //    pRadioButton_Highlight->SetBackgroundColour(face);
   // #endif
   //
   // pToolBar->Realize();

   mLyricsPanel = safenew LyricsPanel(this, -1, parent, panelPos, panelSize);
   RTL_WORKAROUND(mLyricsPanel);

   // vvv Highlight style is broken in ported version.
   // switch (mLyricsPanel->GetLyricsStyle())
   //{
   //    case LyricsPanel::kBouncingBallLyrics:
   //       pRadioButton_BouncingBall->SetValue(true); break;
   //    case LyricsPanel::kHighlightLyrics:
   //    default:
   //       pRadioButton_Highlight->SetValue(true); break;
   // }

   // Events from the project don't propagate directly to this other frame, so...
   if (pProject)
   {
      mSubscription = ProjectWindow::Get(*pProject)
         .GetPlaybackScroller()
         .Subscribe(*this, &LyricsWindow::OnTimer);
   }

   Center();
}

void LyricsWindow::OnCloseWindow(wxCloseEvent & /* event */)
{
   this->Hide();
}

void LyricsWindow::OnStyle_BouncingBall(wxCommandEvent & /* event */)
{
   mLyricsPanel->SetLyricsStyle(LyricsPanel::kBouncingBallLyrics);
}

void LyricsWindow::OnStyle_Highlight(wxCommandEvent & /* event */)
{
   mLyricsPanel->SetLyricsStyle(LyricsPanel::kHighlightLyrics);
}

void LyricsWindow::OnTimer(Observer::Message)
{
   if (auto pProject = mProject.lock())
   {
      if (ProjectAudioIO::Get(*pProject).IsAudioActive())
      {
         auto gAudioIO = AudioIO::Get();
         GetLyricsPanel()->Update(gAudioIO->GetStreamTime());
      }
      else
      {
         // Reset lyrics display.
         const auto &selectedRegion = ViewInfo::Get(*pProject).selectedRegion;
         GetLyricsPanel()->Update(selectedRegion.t0());
      }
   }
}

void LyricsWindow::SetWindowTitle()
{
   wxString name;
   if (auto pProject = mProject.lock())
   {
      name = pProject->GetProjectName();
      if (!name.empty())
         name.Prepend(wxT(" - "));
   }

   SetTitle(AudacityKaraokeTitle.Format(name).Translation());
}

void LyricsWindow::UpdatePrefs()
{
   SetWindowTitle();
}

// Remaining code hooks this add-on into the application
#include "commands/CommandContext.h"
#include "commands/CommandManager.h"

namespace
{

   // Lyrics window attached to each project is built on demand by:
   AttachedWindows::RegisteredFactory sLyricsWindowKey{
       [](TenacityProject &parent) -> wxWeakRef<wxWindow>
       {
          return safenew LyricsWindow(&parent);
       }};

   // Define our extra menu item that invokes that factory
   struct Handler : CommandHandlerObject
   {
      void OnKaraoke(const CommandContext &context)
      {
         auto &project = context.project;

         auto lyricsWindow = &GetAttachedWindows(project).Get(sLyricsWindowKey);
         lyricsWindow->Show();
         lyricsWindow->Raise();
      }
   };

   CommandHandlerObject &findCommandHandler(TenacityProject &)
   {
      // Handler is not stateful.  Doesn't need a factory registered with
      // TenacityProject.
      static Handler instance;
      return instance;
   }

   // Register that menu item

   using namespace MenuTable;
   AttachedItem sAttachment{wxT("View/Windows"),
                            (FinderScope{findCommandHandler},
                             Command(wxT("Karaoke"), XXO("&Karaoke..."), &Handler::OnKaraoke,
                                     LabelTracksExistFlag()))};

}
