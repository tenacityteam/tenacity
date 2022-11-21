/**********************************************************************

  Audacity: A Digital Audio Editor

  LyricsWindow.h

  Vaughan Johnson
  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_LYRICS_WINDOW__
#define __AUDACITY_LYRICS_WINDOW__

#include <wx/frame.h> // to inherit
#include <memory>

// Tenacity libraries
#include <lib-preferences/Prefs.h>

class TenacityProject;
class LyricsPanel;

class LyricsWindow final : public wxFrame,
                           public PrefsListener
{

 public:
   LyricsWindow(TenacityProject* parent);

   LyricsPanel *GetLyricsPanel() { return mLyricsPanel; };

 private:
   void OnCloseWindow(wxCloseEvent & WXUNUSED(event));

   void OnStyle_BouncingBall(wxCommandEvent &evt);
   void OnStyle_Highlight(wxCommandEvent &evt);
   void OnTimer(wxCommandEvent &event);

   void SetWindowTitle();

   // PrefsListener implementation
   void UpdatePrefs() override;

   std::weak_ptr<TenacityProject> mProject;
   LyricsPanel *mLyricsPanel;

 public:
   DECLARE_EVENT_TABLE()
};

#endif
