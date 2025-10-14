/**********************************************************************

  Tenacity

  TrackControlPanel.h

  Avery King

**********************************************************************/

#pragma once

#include "Track.h"
#include "wxPanelWrapper.h"

/** @brief The panel left of a track in Tenacity's track panel.
 *
 * This panel contains the mute and solo buttons, the track info text, and
 * the sliders.
 *
 * A TrackControlPanel always has a track and a project. When a track is
 * removed, this object begins destruction.
 */
class TrackControlPanel : public wxPanelWrapper
{
    private:
        AudacityProject& mProject;
        std::shared_ptr<Track> mTrack;

    public:
        TrackControlPanel(
            wxWindow* parent,
            AudacityProject& project,
            std::shared_ptr<Track>& track
        );

        /** @brief Returns the track associated to the control panel.
         *
         * This is useful when, for example, @ref TrackPanel needs to remove a
         * track.
         */
        const Track& GetTrack() const;
};
