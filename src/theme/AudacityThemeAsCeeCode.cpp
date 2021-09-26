/*!********************************************************************

  Tenacity

  @file AudacityThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudacityThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacity", XO("Audacity") }, ImageCacheAsData
};
