/**********************************************************************

Audacity: A Digital Audio Editor

LabelDefaultClickHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/


#include "LabelDefaultClickHandle.h"

#include "LabelTrackView.h"
#include "../../../HitTestResult.h"
#include "../../../LabelTrack.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"

LabelDefaultClickHandle::LabelDefaultClickHandle()
{
}

LabelDefaultClickHandle::~LabelDefaultClickHandle()
{
}

struct LabelDefaultClickHandle::LabelState {
   std::vector<
      std::pair< std::weak_ptr<LabelTrack>, LabelTrackView::Flags >
   > mPairs;
};

void LabelDefaultClickHandle::SaveState( TenacityProject *pProject )
{
   mLabelState = std::make_shared<LabelState>();
   auto &pairs = mLabelState->mPairs;
   auto &tracks = TrackList::Get( *pProject );

   for (auto lt : tracks.Any<LabelTrack>()) {
      auto &view = LabelTrackView::Get( *lt );
      pairs.push_back( std::make_pair(
         lt->SharedPointer<LabelTrack>(), view.SaveFlags() ) );
   }
}

void LabelDefaultClickHandle::RestoreState( TenacityProject *pProject )
{
   if ( mLabelState ) {
      for ( const auto &pair : mLabelState->mPairs )
         if (auto pLt = TrackList::Get( *pProject ).Lock(pair.first)) {
            auto &view = LabelTrackView::Get( *pLt );
            view.RestoreFlags( pair.second );
         }
      mLabelState.reset();
   }
}

UIHandle::Result LabelDefaultClickHandle::Click
(const TrackPanelMouseEvent &evt, TenacityProject *pProject)
{
   using namespace RefreshCode;
   // Redraw to show the change of text box selection status
   UIHandle::Result result = RefreshAll;

   if (evt.event.LeftDown())
   {
      SaveState( pProject );

      const auto pLT = evt.pCell.get();
      for (auto lt : TrackList::Get( *pProject ).Any<LabelTrack>()) {
         if (pLT != &TrackView::Get( *lt )) {
            auto &view = LabelTrackView::Get( *lt );
            view.ResetFlags();
         }
      }
   }

   return result;
}

UIHandle::Result LabelDefaultClickHandle::Drag
(const TrackPanelMouseEvent &/* evt */, TenacityProject* /* pProject */)
{
   return RefreshCode::RefreshNone;
}

UIHandle::Result LabelDefaultClickHandle::Release
(const TrackPanelMouseEvent &/* evt */, TenacityProject* /* pProject */,
 wxWindow* /* pParent */)
{
   mLabelState.reset();
   return RefreshCode::RefreshNone;
}

UIHandle::Result LabelDefaultClickHandle::Cancel(TenacityProject *pProject)
{
   UIHandle::Result result = RefreshCode::RefreshNone;
   RestoreState( pProject );
   return result;
}
