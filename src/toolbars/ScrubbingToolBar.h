/**********************************************************************

 Audacity: A Digital Audio Editor


 ScrubbingToolbar.h
 
 Paul Licameli

 **********************************************************************/

#ifndef __AUDACITY_SCRUBBING_TOOLBAR__
#define __AUDACITY_SCRUBBING_TOOLBAR__



#include <wx/defs.h>

#include "ToolBar.h"

class TenacityProject;

class wxCommandEvent;
class wxDC;
class wxImage;
class wxWindow;

class AButton;

enum {
   STBScrubID,
   STBSeekID,
   STBRulerID,

   STBNumButtons,
   STBFirstButton = STBScrubID
};

class ScrubbingToolBar final : public ToolBar {

public:

   ScrubbingToolBar( TenacityProject &project );
   virtual ~ScrubbingToolBar();

   static ScrubbingToolBar &Get( TenacityProject &project );
   static const ScrubbingToolBar &Get( const TenacityProject &project );

   void Create(wxWindow *parent) override;

   void OnButton(wxCommandEvent & event);

   void Populate() override;
   void Repaint(wxDC * /* dc */) override {};
   void EnableDisableButtons() override;
   void UpdatePrefs() override;

   void RegenerateTooltips() override;

private:

   static AButton *AddButton(
      ScrubbingToolBar *pBar,
      teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
      int id, const TranslatableString &label, bool toggle = false);

   void MakeButtons();

   AButton *mButtons[STBNumButtons];

   wxImage *upImage;
   wxImage *downImage;
   wxImage *hiliteImage;

   void OnIdle( wxIdleEvent &evt );

   DECLARE_EVENT_TABLE()

private:
   void DoRegenerateTooltips( bool force );

   bool mLastScrub{ false };
   bool mLastSeek{ false };
   bool mLastRuler{ false };
};

#endif
