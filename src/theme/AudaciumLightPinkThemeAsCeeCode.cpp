/*!********************************************************************

  Tenacity

  @file AudaciumLightPinkThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything new)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumLightPinkThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-light-pink", XO("Audacium Light Pink") }, { ImageCacheAsData, false /* is default */}
};
