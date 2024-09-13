/**********************************************************************

  Tenacity

  ZoomToolBar.h

  Avery King split from EditToolBar.h
  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

**********************************************************************/

#pragma once

#include <wx/defs.h>

#include "ToolBar.h"

class wxCommandEvent;
class wxDC;
class wxImage;
class wxWindow;

class AButton;

enum
{
   ZTBZoomInID,
   ZTBZoomOutID,
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
   ZTBZoomToggleID,
#endif

   ZTBZoomSelID,
   ZTBZoomFitID,

   ZTBNumButtons
};

const int first_ZTB_ID = 11300;

// flags so 1,2,4,8 etc.
enum
{
   ZTBActTooltips = 1,
   ZTBActEnableDisable = 2,
};

/** @brief A ToolBar that has the zoom buttons on it.
 * 
 * This class, which is a child of Toolbar, creates the window containing
 * interfaces to commonly-used zoom functions that are otherwise only available
 * through menus. The window can be embedded within a normal project window, or
 * within a ToolBarFrame.
 * 
 * All of the controls in this window were custom-written for Tenacity - they
 * are not native controls on any platform - however, it is intended that the
 * images could be easily replaced to allow "skinning" or just customization to
 * match the look and feel of each platform.
 */
class ZoomToolBar final : public ToolBar
{

public:
   ZoomToolBar(TenacityProject &project);
   virtual ~ZoomToolBar();

   void Create(wxWindow *parent) override;

   void OnButton(wxCommandEvent &event);

   void Populate() override;
   void Repaint(wxDC * /* dc */) override {};
   void EnableDisableButtons() override;
   void UpdatePrefs() override;

private:
   static AButton *AddButton(
       ZoomToolBar *pBar,
       teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
       int id, const TranslatableString &label, bool toggle = false);

   void AddSeparator();

   void MakeButtons();

   void RegenerateTooltips() override;
   void ForAllButtons(int Action);

   AButton *mButtons[ZTBNumButtons];

   wxImage *upImage;
   wxImage *downImage;
   wxImage *hiliteImage;

   DECLARE_EVENT_TABLE()
};
