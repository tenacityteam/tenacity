/**********************************************************************

  Tenacity

  TrackControlPanel.cpp

  Avery King

**********************************************************************/

#include "TrackControlPanel.h"

#include "AllThemeResources.h"
#include "ThemeLegacy.h"
#include "TrackUtilities.h"
#include "widgets/AButton.h"
#include "widgets/auStaticText.h"

#include <wx/button.h>
#include <wx/sizer.h>

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

    auStaticText* trackNameText = new auStaticText(
        this, mTrack->GetName()
    );

    // Setup our panel layout
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(closeButton, 0, wxEXPAND | wxALL);
    sizer->Add(trackNameText, 0, wxEXPAND | wxALL);

    SetSizer(sizer);
    Layout();
}
