/**********************************************************************

  Tenacity

  TrackControlPanel.h

  Avery King

**********************************************************************/

#pragma once

#include "Track.h"
#include "wxPanelWrapper.h"
#include <wx/menu.h>

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

        wxMenu mTrackOptionsMenu;

        void OnClose(wxCommandEvent&);
        void OnTrackOptionsDropdown(wxCommandEvent&);
        void OnTrackMove(wxCommandEvent& event);

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
        const std::shared_ptr<Track>& GetTrack() const;

        /** @brief Adds an option to the track options menu.
         * 
         * The track options menu is a button that contains the name of the
         * track. When clicked, the options menu displays a list of options the
         * user can do on the track.
         *
         * By default, this class adds the following options:
         *   - Rename Track...
         *   - (new section)
         *   - Move Track Up
         *   - Move Track Down
         *   - Move Track to Top
         *   - Move Track to Bottom
         *
         * @param name The name of the new menu entry.
         * @param id The ID of the menu item. Ideally, this should be some
         * proper ID and not the default wxID_ANY.
         * @param action The event handler that will be called when the action
         * is selected. Be sure to skip the event to allow processing of other
         * event handlers too.
         */
        template<class Action>
        void AddOption(TranslatableString name, Action action, int id)
        {
            mTrackOptionsMenu.Append(id, name.Translation());
            Bind(wxEVT_MENU, action, id);
        }

        template<typename Class, typename EventArg, typename Handler>
        void AddOption(TranslatableString name, void(Class::*memberFunction)(EventArg&), Handler* handler, int id = wxID_ANY)
        {
            mTrackOptionsMenu.Append(id, name.Translation());
            Bind(wxEVT_MENU, memberFunction, handler, id);
        }

        /// Adds a separator in the track options menu.
        void AddOptionsSeparator();
};
