/**********************************************************************

Audacity: A Digital Audio Editor

ViewInfo.cpp

Paul Licameli

**********************************************************************/

#include "ViewInfo.h"



#include <algorithm>
#include "XMLAttributeValueView.h"

#include "BasicUI.h"
#include "Prefs.h"
#include "Project.h"

// Tenacity libraries
#include "Prefs.h"
#include "XMLWriter.h"

XMLMethodRegistryBase::Mutators<NotifyingSelectedRegion>
NotifyingSelectedRegion::Mutators(
   const char *legacyT0Name, const char *legacyT1Name)
{
   XMLMethodRegistryBase::Mutators<NotifyingSelectedRegion> results;
   // Get serialization methods of contained SelectedRegion, and wrap each
   for (auto &delegate: SelectedRegion::Mutators(legacyT0Name, legacyT1Name)) {
      results.emplace_back(
         delegate.first,
         [fn = std::move(delegate.second)](auto &region, auto value) {
            fn( region.mRegion, value );
            region.Notify( true );
         }
      );
   }
   return results;
}

NotifyingSelectedRegion& NotifyingSelectedRegion::operator =
( const SelectedRegion &other )
{
   if ( mRegion != other ) {
      mRegion = other;
      Notify();
   }
   return *this;
}

bool NotifyingSelectedRegion::setTimes(double t0, double t1)
{
   bool result = false;
   if ( mRegion.t0() != t0 || mRegion.t1() != t1 ) {
      result = mRegion.setTimes( t0, t1 );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setT0(double t, bool maySwap)
{
   bool result = false;
   if ( mRegion.t0() != t ) {
      result = mRegion.setT0( t, maySwap );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setT1(double t, bool maySwap)
{
   bool result = false;
   if ( mRegion.t1() != t ) {
      result = mRegion.setT1( t, maySwap );
      Notify();
   }
   return result;
}

void NotifyingSelectedRegion::collapseToT0()
{
   if ( mRegion.t0() !=  mRegion.t1() ) {
      mRegion.collapseToT0();
      Notify();
   }
}

void NotifyingSelectedRegion::collapseToT1()
{
   if ( mRegion.t0() !=  mRegion.t1() ) {
      mRegion.collapseToT1();
      Notify();
   }
}

void NotifyingSelectedRegion::move(double delta)
{
   if (delta != 0) {
      mRegion.move( delta );
      Notify();
   }
}

bool NotifyingSelectedRegion::setFrequencies(double f0, double f1)
{
   bool result = false;
   if ( mRegion.f0() != f0 || mRegion.f1() != f1 ) {
      result = mRegion.setFrequencies( f0, f1 );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setF0(double f, bool maySwap)
{
   bool result = false;
   if ( mRegion.f0() != f ) {
      result = mRegion.setF0( f, maySwap );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setF1(double f, bool maySwap)
{
   bool result = false;
   if ( mRegion.f1() != f ) {
      result = mRegion.setF1( f, maySwap );
      Notify();
   }
   return result;
}

void NotifyingSelectedRegion::Notify( bool delayed )
{
   if (delayed)
      GenericUI::CallAfter([This = wxWeakRef(this)]{
         if (This)
            This->Publish({});
      });
   else
      Publish({});
}

static const TenacityProject::AttachedObjects::RegisteredFactory key{
   []( TenacityProject &project ) {
      return std::make_unique<ViewInfo>(0.0, 1.0, ZoomInfo::GetDefaultZoom());
   }
};

ViewInfo &ViewInfo::Get( TenacityProject &project )
{
   return project.AttachedObjects::Get< ViewInfo >( key );
}

const ViewInfo &ViewInfo::Get( const TenacityProject &project )
{
   return Get( const_cast< TenacityProject & >( project ) );
}

ViewInfo::ViewInfo(double start, double screenDuration, double pixelsPerSecond)
   : ZoomInfo(start, pixelsPerSecond)
   , selectedRegion()
   , total(screenDuration)
   , sbarH(0)
   , sbarScreen(1)
   , sbarTotal(1)
   , sbarScale(1.0)
   , scrollStep(16)
   , bUpdateTrackIndicator(true)
   , bScrollBeyondZero(false)
{
   UpdatePrefs();
}

void ViewInfo::UpdateSelectedPrefs( int id )
{
   if (id == UpdateScrollPrefsID())
      gPrefs->Read(wxT("/GUI/AutoScroll"), &bUpdateTrackIndicator,
                   true);
   ZoomInfo::UpdateSelectedPrefs( id );
}

void ViewInfo::UpdatePrefs()
{
   ZoomInfo::UpdatePrefs();
#ifdef EXPERIMENTAL_SCROLLING_LIMITS
   bScrollBeyondZero = ScrollingPreference.Read();
#endif
   gPrefs->Read(wxT("/GUI/AdjustSelectionEdges"), &bAdjustSelectionEdges,
      true);

   UpdateSelectedPrefs( UpdateScrollPrefsID() );
}

void ViewInfo::SetBeforeScreenWidth(wxInt64 beforeWidth, wxInt64 screenWidth, double lowerBoundTime)
{
   h =
      std::max(lowerBoundTime,
         std::min(total - screenWidth / zoom,
         beforeWidth / zoom));
}

void ViewInfo::WriteXMLAttributes(XMLWriter &xmlFile) const
// may throw
{
   selectedRegion.WriteXMLAttributes(xmlFile, "sel0", "sel1");
   xmlFile.WriteAttr(wxT("vpos"), vpos);
   xmlFile.WriteAttr(wxT("h"), h, 10);
   xmlFile.WriteAttr(wxT("zoom"), zoom, 10);
}

//! Construct once at static intialization time to hook project file IO
static struct ViewInfo::ProjectFileIORegistration {

ProjectFileIORegistry::AttributeReaderEntries entries {
[](TenacityProject &project) -> NotifyingSelectedRegion &
{
   return ViewInfo::Get(project).selectedRegion;
},
NotifyingSelectedRegion::Mutators("sel0", "sel1")
};

ProjectFileIORegistry::AttributeReaderEntries entries2 {
// Just a pointer to function, but needing overload resolution as non-const:
(ViewInfo& (*)(TenacityProject &)) &ViewInfo::Get,
{
   { "vpos", [](auto &viewInfo, auto value){
      viewInfo.vpos = value.Get(viewInfo.vpos);
      // Note that (other than in import of old .aup files) there is no other
      // reassignment of vpos, except in handling the vertical scroll.
   } },
   { "h", [](auto &viewInfo, auto value){
      viewInfo.h = value.Get(viewInfo.h);
   } },
   { "zoom", [](auto &viewInfo, auto value){
      viewInfo.zoom = value.Get(viewInfo.zoom);
   } },
} };

} projectFileIORegistration;

int ViewInfo::UpdateScrollPrefsID()
{
   return 10000;
}

static ProjectFileIORegistry::AttributeWriterEntry entry {
[](const TenacityProject &project, XMLWriter &xmlFile){
   ViewInfo::Get(project).WriteXMLAttributes(xmlFile);
}
};

BoolSetting ScrollingPreference{ L"/GUI/ScrollBeyondZero", false };
