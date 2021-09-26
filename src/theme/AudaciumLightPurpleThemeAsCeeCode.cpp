/*!********************************************************************

  Tenacity

  @file AudaciumLightPurpleThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything new)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumLightPurpleThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-light-purple", XO("Audacium Light Purple") }, ImageCacheAsData
};
