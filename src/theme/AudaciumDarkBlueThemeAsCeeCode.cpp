/*!********************************************************************

  Tenacity

  @file AudaciumDarkBlueThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumDarkBlueThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-dark-blue", XO("Audacium Dark Blue") }, { ImageCacheAsData, false /* is default */}
};
