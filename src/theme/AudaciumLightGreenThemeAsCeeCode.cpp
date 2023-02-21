/*!********************************************************************

  Tenacity

  @file AudaciumLightGreenThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything new)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumLightGreenThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-light-green", XO("Audacium Light Green") }, { ImageCacheAsData, false /* is default */}
};
