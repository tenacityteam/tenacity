/**********************************************************************

  Audacity: A Digital Audio Editor


  EditToolbar.h

  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_EDIT_TOOLBAR__
#define __AUDACITY_EDIT_TOOLBAR__

#include <wx/defs.h>

#include "ToolBar.h"

class wxCommandEvent;
class wxDC;
class wxImage;
class wxWindow;

class AButton;

enum {
   ETBCutID,
   ETBCopyID,
   ETBPasteID,
   ETBTrimID,
   ETBSilenceID,

   ETBUndoID,
   ETBRedoID,

#ifdef EXPERIMENTAL_SYNC_LOCK
   //Undefined, so no sync-lock on/off button.
   //#define OPTION_SYNC_LOCK_BUTTON
#endif

#ifdef OPTION_SYNC_LOCK_BUTTON
   ETBSyncLockID,
#endif

   ETBNumButtons
};

const int first_ETB_ID = 11300;

// flags so 1,2,4,8 etc.
enum {
   ETBActTooltips = 1,
   ETBActEnableDisable = 2,
};

class EditToolBar final : public ToolBar {

 public:

   EditToolBar( TenacityProject &project );
   virtual ~EditToolBar();

   void Create(wxWindow *parent) override;

   void OnButton(wxCommandEvent & event);

   void Populate() override;
   void Repaint(wxDC * /* dc */) override {};
   void EnableDisableButtons() override;
   void UpdatePrefs() override;

 private:

   static AButton *AddButton(
      EditToolBar *pBar,
      teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
      int id, const TranslatableString &label, bool toggle = false);

   void AddSeparator();

   void MakeButtons();

   void RegenerateTooltips() override;
   void ForAllButtons(int Action);

   AButton *mButtons[ETBNumButtons];

   wxImage *upImage;
   wxImage *downImage;
   wxImage *hiliteImage;

   DECLARE_EVENT_TABLE()
};

#endif

