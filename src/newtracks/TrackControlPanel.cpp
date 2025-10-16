/**********************************************************************

  Tenacity

  TrackControlPanel.cpp

  Avery King

**********************************************************************/

#include "TrackControlPanel.h"

#include "AllThemeResources.h"
#include "ProjectHistory.h"
#include "ThemeLegacy.h"
#include "Track.h"
#include "TrackUtilities.h"

#include "../commands/SetTrackNameCommand.h"
#include "widgets/AButton.h"
#include "widgets/auStaticText.h"

#include <wx/button.h>
#include <wx/gdicmn.h>
#include <wx/log.h>
#include <wx/sizer.h>

// Event IDs
enum {
    CloseButtonID = 1000,
    TrackDropdownOptionsID
};

enum {
    TrackRenameID = 2000,
    TrackMoveUpID,
    TrackMoveDownID,
    TrackMoveTopID,
    TrackMoveBottomID
};

TrackControlPanel::TrackControlPanel(
    wxWindow* parent,
    AudacityProject& project,
    std::shared_ptr<Track>& track
)
: wxPanelWrapper(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SIMPLE),
  mProject{project},
  mTrack{track}
{
    // For debugging purposes
    SetBackgroundColour(*wxRED);

    // Setup our widgets
    AButton* closeButton = new AButton(
        this, CloseButtonID, wxDefaultPosition, wxDefaultSize,
        theTheme.Image(bmpCloseNormal),
        theTheme.Image(bmpCloseHover),
        theTheme.Image(bmpCloseDown),
        theTheme.Image(bmpCloseHover),
        theTheme.Image(bmpCloseDisabled),
        false
    );

    AButton* trackOptionsDropdown = new AButton(
        this, TrackDropdownOptionsID, wxDefaultPosition, wxDefaultSize,
        theTheme.Image(bmpUpButtonExpand),
        theTheme.Image(bmpUpButtonExpandSel),
        theTheme.Image(bmpHiliteButtonExpand),
        theTheme.Image(bmpHiliteButtonExpandSel),
        theTheme.Image(bmpUpButtonExpand),
        false
    );

    trackOptionsDropdown->SetButtonType(AButton::TextButton);
    trackOptionsDropdown->SetLabel(Verbatim(mTrack->GetName()));

    // Setup our panel layout
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(closeButton, 0, wxEXPAND | wxALL);
    sizer->Add(trackOptionsDropdown, 0, wxEXPAND | wxALL);

    SetSizer(sizer);
    Layout();

    // Setup our popup menu defaults
    // Note: derivatives are free to add their own 
    AddOption(
        XO("Rename Track..."),
        [this, trackOptionsDropdown](wxCommandEvent& event) {
            const wxString oldName = mTrack->GetName();

            SetTrackNameCommand Command;
            Command.mName = oldName;
            // Bug 1837 : We need an OK/Cancel result if we are to enter a blank string.
            bool bResult = Command.PromptUser(mProject);
            if (bResult)
            {
                wxString newName = Command.mName;
                mTrack->SetName(newName);
                trackOptionsDropdown->SetLabel(Verbatim(newName));

                ProjectHistory::Get(mProject)
                    .PushState(
                        XO("Renamed '%s' to '%s'").Format( oldName, newName ),
                        XO("Name Change"));
            }

            // Ensure other handlers process this event too.
            event.Skip();
        }, TrackRenameID
    );

    AddOptionsSeparator();

    AddOption(
        XO("Move Track Up"), &TrackControlPanel::OnTrackMove,
        this, TrackMoveUpID
    );

    AddOption(
        XO("Move Track Down"), &TrackControlPanel::OnTrackMove,
        this, TrackMoveDownID
    );

    AddOption(
        XO("Move Track to Top"), &TrackControlPanel::OnTrackMove,
        this, TrackMoveTopID
    );

    AddOption(
        XO("Move Track to Bottom"), &TrackControlPanel::OnTrackMove,
        this, TrackMoveBottomID
    );

    // Setup our events
    closeButton->Bind(wxEVT_BUTTON, &TrackControlPanel::OnClose, this);
    trackOptionsDropdown->Bind(wxEVT_BUTTON, &TrackControlPanel::OnTrackOptionsDropdown, this);
}

const std::shared_ptr<Track>& TrackControlPanel::GetTrack() const
{
    return mTrack;
}

void TrackControlPanel::AddOptionsSeparator()
{
    mTrackOptionsMenu.AppendSeparator();
}

// Event handlers

void TrackControlPanel::OnClose(wxCommandEvent&)
{
    TrackUtilities::DoRemoveTrack(mProject, *mTrack);
}

void TrackControlPanel::OnTrackOptionsDropdown(wxCommandEvent&)
{
    PopupMenu(&mTrackOptionsMenu);
}

void TrackControlPanel::OnTrackMove(wxCommandEvent& event)
{
    TrackUtilities::MoveChoice choice;
    switch (event.GetId()) {
        case TrackMoveUpID:
            choice = TrackUtilities::OnMoveUpID;
            break;
        case TrackMoveDownID:
            choice = TrackUtilities::OnMoveDownID;
            break;
        case TrackMoveTopID:
            choice = TrackUtilities::OnMoveTopID;
            break;
        case TrackMoveBottomID:
            choice = TrackUtilities::OnMoveBottomID;
            break;
        default:
            // If, for some reason, we reach here, log it and don't perform any
            // track move.
            wxLogError("Invalid track ID. This is likely a bug");
            return;
    }

    TrackUtilities::DoMoveTrack(mProject, *mTrack, choice);
    // event.Skip();
}
