/**********************************************************************

Audacity: A Digital Audio Editor

LabelTextHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_TEXT_HANDLE__
#define __AUDACITY_LABEL_TEXT_HANDLE__

#include "LabelDefaultClickHandle.h"
#include "../../../SelectedRegion.h"

class wxMouseState;
class LabelTrack;
class NotifyingSelectedRegion;
class SelectionStateChanger;
class ZoomInfo;

class LabelTextHandle final : public LabelDefaultClickHandle
{
   static HitTestPreview HitPreview();

public:
   static UIHandlePtr HitTest
      (std::weak_ptr<LabelTextHandle> &holder,
       const wxMouseState &state, const std::shared_ptr<LabelTrack> &pLT);

   LabelTextHandle &operator=(const LabelTextHandle&) = default;

   explicit LabelTextHandle
      ( const std::shared_ptr<LabelTrack> &pLT, int labelNum );
   virtual ~LabelTextHandle();

   std::shared_ptr<LabelTrack> GetTrack() const { return mpLT.lock(); }
   int GetLabelNum() const { return mLabelNum; }

   void Enter(bool forward, TenacityProject *) override;

   bool HandlesRightClick() override;

   Result Click
      (const TrackPanelMouseEvent &event, TenacityProject *pProject) override;

   Result Drag
      (const TrackPanelMouseEvent &event, TenacityProject *pProject) override;

   HitTestPreview Preview
      (const TrackPanelMouseState &state, TenacityProject *pProject)
      override;

   Result Release
      (const TrackPanelMouseEvent &event, TenacityProject *pProject,
       wxWindow *pParent) override;

   Result Cancel(TenacityProject *pProject) override;

private:
   void HandleTextClick
      (TenacityProject &project, const wxMouseEvent & evt);
   void HandleTextDragRelease(
      TenacityProject &project, const wxMouseEvent & evt);

   std::weak_ptr<LabelTrack> mpLT {};
   int mLabelNum{ -1 };
   int mLabelTrackStartXPos { -1 };
   int mLabelTrackStartYPos { -1 };

   /// flag to tell if it's a valid dragging
   bool mRightDragging{ false };
};

#endif
