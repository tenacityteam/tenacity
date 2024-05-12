/**********************************************************************

  Audacity: A Digital Audio Editor

  ZoomInfo.cpp

  Paul Licameli split from ViewInfo.cpp

**********************************************************************/

#include "ZoomInfo.h"
#include "Decibels.h"

#include <cmath>
#include <limits>

namespace {
static const double gMaxZoom = 6000000;
static const double gMinZoom = 0.001;
}

ZoomInfo::ZoomInfo(double start, double pixelsPerSecond)
   : vpos(0)
   , h(start)
   , zoom(pixelsPerSecond)
{
   UpdatePrefs();
}

ZoomInfo::~ZoomInfo()
{
}

void ZoomInfo::UpdatePrefs()
{
   dBr = DecibelScaleCutoff.Read();
}

/// Converts a position (mouse X coordinate) to
/// project time, in seconds.  Needs the left edge of
/// the track as an additional parameter.
double ZoomInfo::PositionToTime(long long position, long long origin) const
{
   return h + (position - origin) / zoom;
}


/// STM: Converts a project time to screen x position.
long long ZoomInfo::TimeToPosition(double projectTime, long long origin) const
{
   double t = 0.5 + zoom * (projectTime - h) + origin ;
   if( (long) t < std::numeric_limits<long long>::min() )
      return std::numeric_limits<long long>::min();
   if( (long) t > std::numeric_limits<long long>::max() )
      return std::numeric_limits<long long>::max();
   t = floor( t );
   return t;
}

// You should prefer to call TimeToPosition twice, for endpoints, and take the difference!
double ZoomInfo::TimeRangeToPixelWidth(double timeRange) const
{
   return timeRange * zoom;
}

bool ZoomInfo::ZoomInAvailable() const
{
   return zoom < gMaxZoom;
}

bool ZoomInfo::ZoomOutAvailable() const
{
   return zoom > gMinZoom;
}

double ZoomInfo::GetZoom( ) const { return zoom;};
double ZoomInfo::GetMaxZoom( ) { return gMaxZoom;};
double ZoomInfo::GetMinZoom( ) { return gMinZoom;};

void ZoomInfo::SetZoom(double pixelsPerSecond)
{
   zoom = std::max(gMinZoom, std::min(gMaxZoom, pixelsPerSecond));
}

void ZoomInfo::ZoomBy(double multiplier)
{
   SetZoom(zoom * multiplier);
}

void ZoomInfo::FindIntervals
   (double /*rate*/, Intervals &results, long long width, long long origin) const
{
   results.clear();
   results.reserve(2);

   const long long rightmost(origin + (0.5 + width));
   assert(origin <= rightmost);
   {
      results.push_back(Interval(origin, zoom, false));
   }

   if (origin < rightmost)
      results.push_back(Interval(rightmost, 0, false));
   assert(!results.empty() && results[0].position == origin);
}
