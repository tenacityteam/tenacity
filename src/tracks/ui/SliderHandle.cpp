/**********************************************************************

Audacity: A Digital Audio Editor

SliderHandle.cpp

Paul Licameli

**********************************************************************/


#include "SliderHandle.h"

#include "../../widgets/ASlider.h"
#include "../../HitTestResult.h"
#include "../../RefreshCode.h"
#include "Track.h"
#include "../../TrackPanelMouseEvent.h"

SliderHandle::SliderHandle
( SliderFn sliderFn, const wxRect &rect, const std::shared_ptr<Track> &pTrack )
   : mpTrack{ pTrack }
   , mRect{ rect }
   , mSliderFn{ sliderFn }
{
}

void SliderHandle::Enter(bool, TenacityProject *)
{
   mChangeHighlight = RefreshCode::RefreshCell;
}

SliderHandle::~SliderHandle()
{
}

UIHandle::Result SliderHandle::Click
(const TrackPanelMouseEvent &evt, TenacityProject *pProject)
{
   wxMouseEvent &event = evt.event;
   using namespace RefreshCode;
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return Cancelled;

   // Come here for left click or double click
   mStartingValue = GetValue();
   auto slider = GetSlider( pProject );
   slider->OnMouseEvent(event);
   const float newValue = slider->Get();

   // Make a non-permanent change to the project data:
   auto result = SetValue(pProject, newValue);

   if (event.ButtonDClick())
      // Just did a modal dialog in OnMouseEvent
      // Do not start a drag
      return result | RefreshCell | Cancelled;
   else {
      mIsClicked = true;
      return result | RefreshCell;
   }
}

UIHandle::Result SliderHandle::Drag
(const TrackPanelMouseEvent &evt, TenacityProject *pProject)
{
   wxMouseEvent &event = evt.event;
   using namespace RefreshCode;
   GetSlider( pProject )->OnMouseEvent(event);
   const float newValue = GetSlider( pProject )->Get();

   // Make a non-permanent change to the project data:
   return RefreshCell | SetValue(pProject, newValue);
}

HitTestPreview SliderHandle::Preview
(const TrackPanelMouseState &st, TenacityProject *project)
{
   // No special cursor
   TranslatableString message;
   if (project)
      message = Tip(st.state, *project);
   return { message, {}, message };
}

UIHandle::Result SliderHandle::Release
(const TrackPanelMouseEvent &evt, TenacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   wxMouseEvent &event = evt.event;
   GetSlider( pProject )->OnMouseEvent(event);
   const float newValue = GetSlider( pProject )->Get();

   Result result = RefreshCell;

   // Commit changes to the project data:
   result |= SetValue(pProject, newValue);
   result |= CommitChanges(event, pProject);

   mpTrack.reset();
   return result;
}

UIHandle::Result SliderHandle::Cancel(TenacityProject *pProject)
{
   wxMouseEvent event(wxEVT_LEFT_UP);
   GetSlider( pProject )->OnMouseEvent(event);

   // Undo un-committed changes to project data:
   auto result = SetValue(pProject, mStartingValue);
   mpTrack.reset();
   return RefreshCode::RefreshCell | result;
}

LWSlider *SliderHandle::GetSlider( TenacityProject *pProject )
{
   auto pTrack = TrackList::Get( *pProject ).Lock(mpTrack);
   return mSliderFn( pProject, mRect, pTrack.get() );
}
