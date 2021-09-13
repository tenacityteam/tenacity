/*!********************************************************************
*
 Audacity: A Digital Audio Editor

 WaveTrackAffordanceControls.h

 Vitaly Sverchinsky

 **********************************************************************/

#pragma once

#include <wx/font.h>
#include <wx/event.h>

#include "../../../ui/CommonTrackPanelCell.h"
#include "../../../ui/TextEditHelper.h"


struct TrackListEvent;

class AffordanceHandle;
class WaveClip;
class TrackPanelResizeHandle;
class WaveClipTitleEditHandle;
class WaveTrackAffordanceHandle;
class TrackList;

//Handles clip movement, selection, navigation and
//allow name change
class SAUCEDACITY_DLL_API WaveTrackAffordanceControls : 
    public CommonTrackCell,
    public TextEditDelegate,
    public wxEvtHandler,
    public std::enable_shared_from_this<WaveTrackAffordanceControls>
{
    std::weak_ptr<WaveClip> mFocusClip;
    std::weak_ptr<WaveTrackAffordanceHandle> mAffordanceHandle;
    std::weak_ptr<TrackPanelResizeHandle> mResizeHandle;
    std::weak_ptr<WaveClipTitleEditHandle> mTitleEditHandle;

    std::shared_ptr<TextEditHelper> mTextEditHelper;

    wxFont mClipNameFont;

public:
    WaveTrackAffordanceControls(const std::shared_ptr<Track>& pTrack);

    std::vector<UIHandlePtr> HitTest(const TrackPanelMouseState& state, const SaucedacityProject* pProject) override;

    void Draw(TrackPanelDrawingContext& context, const wxRect& rect, unsigned iPass) override;

    //Invokes name editing for a clip that currently is
    //in focus(as a result of hit testing), returns true on success
    //false if there is no focus
    bool StartEditClipName(SaucedacityProject* project);

    std::weak_ptr<WaveClip> GetSelectedClip() const;

    unsigned CaptureKey
    (wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
        SaucedacityProject* project) override;
    
    unsigned KeyDown (wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
        SaucedacityProject* project) override;

    unsigned Char
    (wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
        SaucedacityProject* project) override;

    void OnTextEditFinished(SaucedacityProject* project, const wxString& text) override;
    void OnTextEditCancelled(SaucedacityProject* project) override;
    void OnTextModified(SaucedacityProject* project, const wxString& text) override;
    void OnTextContextMenu(SaucedacityProject* project, const wxPoint& position) override;

private:
    void OnTrackChanged(TrackListEvent& evt);

    bool SelectNextClip(ViewInfo& viewInfo, SaucedacityProject* project, bool forward);
    bool StartEditSelectedClipName(ViewInfo& viewInfo, SaucedacityProject* project);

    std::shared_ptr<TextEditHelper> MakeTextEditHelper(const wxString& text);
};
