/*!********************************************************************
*
 Audacity: A Digital Audio Editor

 WaveClipTrimHandle.h

 Vitaly Sverchinsky

 **********************************************************************/

#pragma once 

#include "UIHandle.h"
#include "WaveClip.h"

class WaveTrackView;
class WaveTrack;

class WaveClipTrimHandle : public UIHandle
{
    static constexpr int BoundaryThreshold = 5;

    static HitTestPreview HitPreview(const TenacityProject*, bool unsafe);

    //Different policies implement different trimming scenarios
    class ClipTrimPolicy
    {
    public:
       virtual ~ClipTrimPolicy();

       virtual bool Init(const TrackPanelMouseEvent& event) = 0;
       virtual void Trim(const TrackPanelMouseEvent& event, TenacityProject& project) = 0;
       virtual void Finish(TenacityProject& project) = 0;
       virtual void Cancel() = 0;
    };
    class AdjustBorder;
    
    std::unique_ptr<ClipTrimPolicy> mClipTrimPolicy;

public:
    WaveClipTrimHandle(std::unique_ptr<ClipTrimPolicy>& clipTrimPolicy);

    static UIHandlePtr HitAnywhere(std::weak_ptr<WaveClipTrimHandle>& holder,
        WaveTrack* waveTrack,
        const TenacityProject* pProject,
        const TrackPanelMouseState& state);

    static UIHandlePtr HitTest(std::weak_ptr<WaveClipTrimHandle>& holder,
        WaveTrackView& view, const TenacityProject* pProject,
        const TrackPanelMouseState& state);

    HitTestPreview Preview(const TrackPanelMouseState& mouseState, TenacityProject* pProject) override;

    virtual Result Click
    (const TrackPanelMouseEvent& event, TenacityProject* pProject) override;

    virtual Result Drag
    (const TrackPanelMouseEvent& event, TenacityProject* pProject) override;

    virtual Result Release
    (const TrackPanelMouseEvent& event, TenacityProject* pProject,
        wxWindow* pParent) override;

    virtual Result Cancel(TenacityProject* pProject) override;
};
