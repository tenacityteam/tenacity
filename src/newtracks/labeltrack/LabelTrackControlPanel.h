/**********************************************************************

  Tenacity

  LabelTrackControlPanel.h

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#pragma once

#include "../TrackControlPanel.h"

/** @brief The control panel for label tracks.
 *
 * Adds a font option to the track options menu and nothing more.
 */
class LabelTrackControlPanel final : public TrackControlPanel
{
    private:
        void OnFontDialog(wxCommandEvent&);

    public:
        LabelTrackControlPanel(
            wxWindow* parent, AudacityProject& project,
            std::shared_ptr<Track>& track
        );
        ~LabelTrackControlPanel() override = default;
};
