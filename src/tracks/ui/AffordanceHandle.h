/*!********************************************************************
*
 Audacity: A Digital Audio Editor

 AffordanceHandle.h

 Vitaly Sverchinsky

 **********************************************************************/

#pragma once

#include "TimeShiftHandle.h"

class TENACITY_DLL_API AffordanceHandle : public TimeShiftHandle
{
    static HitTestPreview HitPreview(const TenacityProject*, bool unsafe, bool moving);
public:

    void Enter(bool forward, TenacityProject* pProject) override;
    HitTestPreview Preview(const TrackPanelMouseState& mouseState, TenacityProject* pProject) override;

    AffordanceHandle(const std::shared_ptr<Track>& track);

    Result Click(const TrackPanelMouseEvent& evt, TenacityProject* pProject) override;
    Result Release(const TrackPanelMouseEvent& event, TenacityProject* pProject, wxWindow* pParent) override;

protected:
    virtual Result SelectAt(const TrackPanelMouseEvent& event, TenacityProject* pProject) = 0;
};
