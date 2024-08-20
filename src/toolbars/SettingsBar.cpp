/**********************************************************************

  Tenacity

  SettingsBar.cpp

  Avery King split from SelectionBar.cpp
  Copyright 2005 Dominic Mazzoni

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

*******************************************************************//**

\class SettingsBar
\brief (not quite a Toolbar) at foot of screen for setting and viewing the
selection range.

*//*******************************************************************/



#include "SettingsBar.h"

#include "SettingsBarListener.h"
#include "ToolManager.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/intl.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/stattext.h>
#endif
#include <wx/statline.h>

// Tenacity libraries
#include <lib-preferences/Prefs.h>
#include <lib-project/Project.h>
#include <lib-project-rate/QualitySettings.h>

#include "../AudioIO.h"
#include "../AColor.h"
#include "../KeyboardCapture.h"
#include "../ProjectAudioIO.h"
#include "../ProjectSettings.h"
#include "../Snap.h"
#include "ViewInfo.h"
#include "../theme/AllThemeResources.h"
#include "../widgets/AuStaticText.h"

#if wxUSE_ACCESSIBILITY
#include "../widgets/WindowAccessible.h"
#endif

IMPLEMENT_CLASS(SettingsBar, ToolBar);

constexpr int SIZER_COLS = 3;

const static wxChar *numbers[] =
{
    wxT("0"), wxT("1"), wxT("2"), wxT("3"), wxT("4"),
    wxT("5"), wxT("6"), wxT("7"), wxT("8"), wxT("9")
};

enum
{
    SettingsBarFirstID = 2700,
    RateID,
    SnapToID,
    OnMenuID,
};

BEGIN_EVENT_TABLE(SettingsBar, ToolBar)
    EVT_SIZE(SettingsBar::OnSize)
    EVT_CHOICE(SnapToID, SettingsBar::OnSnapTo)
    EVT_COMBOBOX(RateID, SettingsBar::OnRate)
    EVT_TEXT(RateID, SettingsBar::OnRate)

    EVT_COMMAND(wxID_ANY, EVT_TIMETEXTCTRL_UPDATED, SettingsBar::OnUpdate)
    EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, SettingsBar::OnCaptureKey)
END_EVENT_TABLE()

SettingsBar::SettingsBar(TenacityProject &project)
    : ToolBar(project, SettingsBarID, XO("Settings"), wxT("Settings")),
      mListener{nullptr}, mRate(0.0)
{
    // Make sure we have a valid rate as the NumericTextCtrl()s
    // created in Populate()
    // depend on it.  Otherwise, division-by-zero floating point exceptions
    // will occur.
    // Refer to bug #462 for a scenario where the division-by-zero causes
    // Audacity to fail.
    // We expect mRate to be set from the project later.
    mRate = (double)QualitySettings::DefaultSampleRate.Read();
}

SettingsBar::~SettingsBar()
{
}

SettingsBar &SettingsBar::Get(TenacityProject &project)
{
    auto &toolManager = ToolManager::Get(project);
    return *static_cast<SettingsBar *>(toolManager.GetToolBar(SettingsBarID));
}

const SettingsBar &SettingsBar::Get(const TenacityProject &project)
{
    return Get(const_cast<TenacityProject &>(project));
}

void SettingsBar::Create(wxWindow *parent)
{
    ToolBar::Create(parent);
    UpdatePrefs();
}

AuStaticText *SettingsBar::AddTitle(
    const TranslatableString &Title, wxSizer *pSizer)
{
    const auto translated = Title.Translation();
    AuStaticText *pTitle = new AuStaticText(this, translated);
    pTitle->SetBackgroundColour(theTheme.Colour(clrMedium));
    pTitle->SetForegroundColour(theTheme.Colour(clrTrackPanelText));
    pSizer->Add(pTitle, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    return pTitle;
}

void SettingsBar::AddVLine(wxSizer *pSizer)
{
    pSizer->Add(new wxStaticLine(this, -1, wxDefaultPosition,
                                     wxSize(1, toolbarSingle - 10),
                                     wxLI_VERTICAL),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
}

void SettingsBar::Populate()
{
    SetBackgroundColour(theTheme.Colour(clrMedium));

    // Outer sizer has space top and left.
    // Inner sizers have space on right only.
    // This choice makes for a nice border and internal spacing and places clear responsibility
    // on each sizer as to what spacings it creates.
    wxFlexGridSizer *mainSizer = new wxFlexGridSizer(SIZER_COLS, 1, 1);
    Add(mainSizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    // Top row (mostly labels)
    wxColour clrText = theTheme.Colour(clrTrackPanelText);
    wxColour clrText2 = *wxBLUE;
    AuStaticText *rateLabel = AddTitle(XO("Project Rate (Hz)"), mainSizer);
    AddVLine(mainSizer);
    AuStaticText *snapLabel = AddTitle(XO("Snap-To"), mainSizer);

    // Bottom row, (mostly time controls)
    mRateBox = new wxComboBox(this, RateID,
                                  wxT(""),
                                  wxDefaultPosition, wxDefaultSize);
#if wxUSE_ACCESSIBILITY
    // so that name can be set on a standard control
    mRateBox->SetAccessible(new WindowAccessible(mRateBox));
#endif
    mRateBox->SetName(_("Project Rate (Hz)"));
    // mRateBox->SetForegroundColour( clrText2 );
    wxTextValidator vld(wxFILTER_INCLUDE_CHAR_LIST);
    vld.SetIncludes(wxArrayString(10, numbers));
    mRateBox->SetValidator(vld);
    mRateBox->SetValue(wxString::Format(wxT("%d"), (int)mRate));
    UpdateRates(); // Must be done _after_ setting value on mRateBox!

    // We need to capture the SetFocus and KillFocus events to set up
    // for keyboard capture.  On Windows and GTK it's easy since the
    // combobox is presented as one control to hook into.
    mRateText = mRateBox;

#if defined(__WXMAC__)
    // The Mac uses a standard wxTextCtrl for the edit portion and that's
    // the control that gets the focus events.  So we have to find the
    // textctrl.
    wxWindowList kids = mRateBox->GetChildren();
    for (unsigned int i = 0; i < kids.size(); i++)
    {
        wxClassInfo *ci = kids[i]->GetClassInfo();
        if (ci->IsKindOf(CLASSINFO(wxTextCtrl)))
        {
            mRateText = kids[i];
            break;
        }
    }
#endif

    mRateText->Bind(wxEVT_SET_FOCUS,
                    &SettingsBar::OnFocus,
                    this);
    mRateText->Bind(wxEVT_KILL_FOCUS,
                    &SettingsBar::OnFocus,
                    this);

    mainSizer->Add(mRateBox, 0, wxEXPAND | wxALIGN_TOP | wxRIGHT, 5);

    AddVLine(mainSizer);

    mSnapTo = new wxChoice(this, SnapToID,
                               wxDefaultPosition, wxDefaultSize,
                               transform_container<wxArrayStringEx>(
                                   SnapManager::GetSnapLabels(),
                                   std::mem_fn(&TranslatableString::Translation)));

#if wxUSE_ACCESSIBILITY
    // so that name can be set on a standard control
    mSnapTo->SetAccessible(new WindowAccessible(mSnapTo));
#endif
    mSnapTo->SetName(_("Snap To"));
    // mSnapTo->SetForegroundColour( clrText2 );
    mSnapTo->SetSelection(mListener ? mListener->AS_GetSnapTo() : SNAP_OFF);

    mSnapTo->Bind(wxEVT_SET_FOCUS,
                  &SettingsBar::OnFocus,
                  this);
    mSnapTo->Bind(wxEVT_KILL_FOCUS,
                  &SettingsBar::OnFocus,
                  this);

    mainSizer->Add(mSnapTo, 0, wxEXPAND | wxALIGN_TOP | wxRIGHT, 5);

    // Make sure they are fully expanded to the longest item
    mRateBox->SetMinSize(wxSize(mRateBox->GetBestSize().x, toolbarSingle));
    mSnapTo->SetMinSize(wxSize(mSnapTo->GetBestSize().x, toolbarSingle));

    // This shows/hides controls.
    // Do this before layout so that we are sized right.
    mainSizer->Layout();
    RegenerateTooltips();
    Layout();
}

void SettingsBar::UpdatePrefs()
{
    // The project rate is no longer driven from here.
    // When preferences change, the Project learns about it too.
    // If necessary we can drive the SettingsBar mRate via the Project
    // calling our SetRate().
    // As of 13-Sep-2018, changes to the sample rate pref will only affect
    // creation of new projects, not the sample rate in existing ones.

    // Set label to pull in language change
    SetLabel(XO("Settings"));

    RegenerateTooltips();
    // Give base class a chance
    ToolBar::UpdatePrefs();
}

void SettingsBar::SetListener(SettingsBarListener *l)
{
    mListener = l;
    SetRate(mListener->AS_GetRate());
    SetSnapTo(mListener->AS_GetSnapTo());
};

void SettingsBar::OnSize(wxSizeEvent &evt)
{
    Refresh(true);

    evt.Skip();
}

// Called when one of the format drop downs is changed.
void SettingsBar::OnUpdate(wxCommandEvent &evt)
{
    evt.Skip(false);

    mRateBox  = nullptr;
    mRateText = nullptr;
    mSnapTo   = nullptr;

    ToolBar::ReCreateButtons();

    RegenerateTooltips();
    Updated();
}

void SettingsBar::SetSnapTo(int snap)
{
    mSnapTo->SetSelection(snap);
}

void SettingsBar::SetRate(double rate)
{
    if (rate != mRate)
    {
        // if the rate is actually being changed
        mRate = rate; // update the stored rate
        mRateBox->SetValue(wxString::Format(wxT("%d"), (int)rate));
    }
}

void SettingsBar::OnRate(wxCommandEvent & /* event */)
{
    auto value = mRateBox->GetValue();

    if (value.ToDouble(&mRate) && // is a numeric value
        (mRate != 0.0))
    {
        if (mListener)
            mListener->AS_SetRate(mRate);

        mLastValidText = value;
    }
    else
    {
        // Bug 2497 - Undo paste into text box if it's not numeric
        mRateBox->SetValue(mLastValidText);
    }
}

void SettingsBar::UpdateRates()
{
    wxString oldValue = mRateBox->GetValue();
    mRateBox->Clear();
    for (int i = 0; i < AudioIOBase::NumStandardRates; i++)
    {
        mRateBox->Append(
            wxString::Format(wxT("%d"), AudioIOBase::StandardRates[i]));
    }
    mRateBox->SetValue(oldValue);
}

void SettingsBar::OnFocus(wxFocusEvent &event)
{
    KeyboardCapture::OnFocus(*this, event);
}

void SettingsBar::OnCaptureKey(wxCommandEvent &event)
{
    wxKeyEvent *kevent = (wxKeyEvent *)event.GetEventObject();
    wxWindow *w = FindFocus();
    int keyCode = kevent->GetKeyCode();

    // Convert numeric keypad entries.
    if ((keyCode >= WXK_NUMPAD0) && (keyCode <= WXK_NUMPAD9))
    {
        keyCode -= WXK_NUMPAD0 - '0';
    }

    if (keyCode >= '0' && keyCode <= '9')
    {
        return;
    }

    // UP/DOWN/LEFT/RIGHT for mRateText
    if (w == mRateText)
    {
        switch (keyCode)
        {
        case WXK_LEFT:
        case WXK_RIGHT:
        case WXK_UP:
        case WXK_DOWN:
        case WXK_DELETE:
        case WXK_BACK:
            return;
        }
    }

    event.Skip();
}

void SettingsBar::OnSnapTo(wxCommandEvent & /* event */)
{
    mListener->AS_SetSnapTo(mSnapTo->GetSelection());
}

static RegisteredToolbarFactory factory{
    SettingsBarID,
    [](TenacityProject &project)
    { return ToolBar::Holder{new SettingsBar{project}}; }
};

namespace {
AttachedToolBarMenuItem sAttachment{
    /* i18n-hint: Clicking this menu item shows the toolbar
        for selecting a time range of audio */
    SettingsBarID, wxT("ShowSettingsTB"), XXO("S&election Toolbar")
};
}
