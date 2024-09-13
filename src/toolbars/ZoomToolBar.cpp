/**********************************************************************

  Tenacity

  ZoomToolBar.cpp

  Avery King split from EditToolBar.cpp
  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

**********************************************************************/

#include "ZoomToolBar.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/tooltip.h>
#endif

// Tenacity libraries
#include <lib-project/Project.h>
#include <lib-preferences/Prefs.h>

#include "../theme/AllThemeResources.h"
#include "../BatchCommands.h"
#include "../ImageManipulation.h"
#include "../Menus.h"
#include "../UndoManager.h"
#include "../widgets/AButton.h"

#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"

constexpr int BUTTON_WIDTH = 27;
constexpr int SEPARATOR_WIDTH = 14;

////////////////////////////////////////////////////////////
/// Methods for ZoomToolBar
////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ZoomToolBar, ToolBar)
    EVT_COMMAND_RANGE(
        ZTBZoomInID + first_ZTB_ID,
        ZTBZoomInID + first_ZTB_ID + ZTBNumButtons - 1,
        wxEVT_COMMAND_BUTTON_CLICKED,
        ZoomToolBar::OnButton
    )
END_EVENT_TABLE()

// Standard constructor
ZoomToolBar::ZoomToolBar(TenacityProject &project)
: ToolBar(project, ZoomBarID, XO("Zoom"), "Zoom")
{
}

ZoomToolBar::~ZoomToolBar()
{
}

void ZoomToolBar::Create(wxWindow *parent)
{
    ToolBar::Create(parent);
    UpdatePrefs();
}

void ZoomToolBar::AddSeparator()
{
    AddSpacer();
}

/// This is a convenience function that allows for button creation in
/// MakeButtons() with fewer arguments
/// Very similar to code in ControlToolBar...
AButton *ZoomToolBar::AddButton(
    ZoomToolBar *pBar,
    teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
    int id,
    const TranslatableString &label,
    bool toggle)
{
    AButton *&r = pBar->mButtons[id];

    r = ToolBar::MakeButton(
        pBar,
        bmpRecoloredUpSmall, bmpRecoloredDownSmall, bmpRecoloredUpHiliteSmall, bmpRecoloredHiliteSmall,
        eEnabledUp, eEnabledDown, eDisabled,
        wxWindowID(id + first_ZTB_ID),
        wxDefaultPosition,
        toggle,
        theTheme.ImageSize(bmpRecoloredUpSmall)
    );

    r->SetLabel(label);
    // JKC: Unlike ControlToolBar, does not have a focus rect.  Shouldn't it?
    // r->SetFocusRect( r->GetRect().Deflate( 4, 4 ) );

    pBar->Add(r, 0, wxALIGN_CENTER);

    return r;
}

void ZoomToolBar::Populate()
{
    SetBackgroundColour(theTheme.Colour(clrMedium));
    MakeButtonBackgroundsSmall();

    /* Buttons */
    // Tooltips match menu entries.
    // We previously had longer tooltips which were not more clear.
    AddButton(this, bmpZoomIn, bmpZoomIn, bmpZoomInDisabled, ZTBZoomInID,
              XO("Zoom In"));
    AddButton(this, bmpZoomOut, bmpZoomOut, bmpZoomOutDisabled, ZTBZoomOutID,
              XO("Zoom Out"));
    AddButton(this, bmpZoomSel, bmpZoomSel, bmpZoomSelDisabled, ZTBZoomSelID,
              XO("Zoom to Selection"));
    AddButton(this, bmpZoomFit, bmpZoomFit, bmpZoomFitDisabled, ZTBZoomFitID,
              XO("Fit to Width"));

#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
    AddButton(this, bmpZoomToggle, bmpZoomToggle, bmpZoomToggleDisabled, ZTBZoomToggleID,
              XO("Zoom Toggle"));
#endif

    mButtons[ZTBZoomInID]->SetEnabled(false);
    mButtons[ZTBZoomOutID]->SetEnabled(false);
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
    mButtons[ZTBZoomToggleID]->SetEnabled(false);
#endif

    mButtons[ZTBZoomSelID]->SetEnabled(false);
    mButtons[ZTBZoomFitID]->SetEnabled(false);

    RegenerateTooltips();
}

void ZoomToolBar::UpdatePrefs()
{
    RegenerateTooltips();

    // Set label to pull in language change
    SetLabel(XO("Edit"));

    // Give base class a chance
    ToolBar::UpdatePrefs();
}

void ZoomToolBar::RegenerateTooltips()
{
    ForAllButtons(ZTBActTooltips);
}

void ZoomToolBar::EnableDisableButtons()
{
    ForAllButtons(ZTBActEnableDisable);
}

static const struct Entry
{
    int tool;
    CommandID commandName;
    TranslatableString untranslatedLabel;
} ZoomToolBarButtonList[] = {
    {ZTBZoomInID,  wxT("ZoomIn"),      XO("Zoom In")},
    {ZTBZoomOutID, wxT("ZoomOut"),     XO("Zoom Out")},
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
    {ZTBZoomToggleID, wxT("ZoomToggle"), XO("Zoom Toggle")},
#endif
    {ZTBZoomSelID, wxT("ZoomSel"),     XO("Fit selection to width")},
    {ZTBZoomFitID, wxT("FitInWindow"), XO("Fit project to width")},
};

void ZoomToolBar::ForAllButtons(int Action)
{
    TenacityProject *p;
    CommandManager *cm = nullptr;

    if (Action & ZTBActEnableDisable)
    {
        p = &mProject;
        cm = &CommandManager::Get(*p);
    }

    for (const auto &entry : ZoomToolBarButtonList)
    {
#if wxUSE_TOOLTIPS
        if (Action & ZTBActTooltips)
        {
            ComponentInterfaceSymbol command{
                entry.commandName,
                entry.untranslatedLabel
            };
            ToolBar::SetButtonToolTip(
                mProject,
                *mButtons[entry.tool],
                &command,
                1u
            );
        }
#endif
        if (cm)
        {
            mButtons[entry.tool]->SetEnabled(cm->GetEnabled(entry.commandName));
        }
    }
}

void ZoomToolBar::OnButton(wxCommandEvent &event)
{
    int id = event.GetId() - first_ZTB_ID;
    // Be sure the pop-up happens even if there are exceptions, except for buttons which toggle.
    auto cleanup = finally([&] {
            mButtons[id]->InteractionOver();
        }
    );

    TenacityProject *p = &mProject;
    auto &cm = CommandManager::Get(*p);

    auto flags = MenuManager::Get(*p).GetUpdateFlags();
    const CommandContext context(*p);
    MacroCommands::HandleTextualCommand(
        cm,
        ZoomToolBarButtonList[id].commandName,
        context,
        flags,
        false
    );

#if defined(__WXMAC__)
    // Bug 2402
    // LLL: It seems that on the Mac the IDLE events are processed
    //      differently than on Windows/GTK and the AdornedRulerPanel's
    //      OnPaint() method gets called sooner that expected. This is
    //      evident when zooming from this toolbar only. When zooming from
    //      the Menu or from keyboard ommand, the zooming works correctly.
    wxTheApp->ProcessIdle();
#endif
}

static RegisteredToolbarFactory factory{
    ZoomBarID,
    [](TenacityProject &project) {
        return ToolBar::Holder{safenew ZoomToolBar{project}};
    }
};

#include "ToolManager.h"

namespace
{
    AttachedToolBarMenuItem sAttachment{
        /* i18n-hint: Clicking this menu item shows the toolbar for editing */
        EditBarID, wxT("ShowZoomTB"), XXO("&Zoom Toolbar")};
}
