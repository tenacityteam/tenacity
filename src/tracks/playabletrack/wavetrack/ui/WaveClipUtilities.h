/**********************************************************************

  Audacity: A Digital Audio Editor

  @file WaveClipUtilities.h

  Paul Licameli split from WaveClip.h

*******************************************************************/

#ifndef __AUDACITY_WAVE_CLIP_UTILITIES__
#define __AUDACITY_WAVE_CLIP_UTILITIES__

#include <vector>

class sampleCount;

TENACITY_DLL_API
void findCorrection(const std::vector<sampleCount> &oldWhere, std::size_t oldLen,
   std::size_t newLen, double t0, double rate, double samplesPerPixel,
   int &oldX0, double &correction);

TENACITY_DLL_API
void fillWhere(std::vector<sampleCount> &where,
   std::size_t len, double bias, double correction,
   double t0, double rate, double samplesPerPixel);

#endif
