/*!********************************************************************
*
 Audacity: A Digital Audio Editor

 AffordanceHandle.h

 Vitaly Sverchinsky

 **********************************************************************/

#pragma once

#include "TimeShiftHandle.h"

class SAUCEDACITY_DLL_API AffordanceHandle : public TimeShiftHandle
{
    static HitTestPreview HitPreview(const SaucedacityProject*, bool unsafe, bool moving);
public:

    void Enter(bool forward, SaucedacityProject* pProject) override;
    HitTestPreview Preview(const TrackPanelMouseState& mouseState, SaucedacityProject* pProject) override;

    AffordanceHandle(const std::shared_ptr<Track>& track);

    Result Click(const TrackPanelMouseEvent& evt, SaucedacityProject* pProject) override;
    Result Release(const TrackPanelMouseEvent& event, SaucedacityProject* pProject, wxWindow* pParent) override;

protected:
    virtual Result SelectAt(const TrackPanelMouseEvent& event, SaucedacityProject* pProject) = 0;
};
