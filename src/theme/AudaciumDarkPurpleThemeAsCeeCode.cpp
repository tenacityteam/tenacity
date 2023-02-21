/*!********************************************************************

  Tenacity

  @file AudaciumDarkPurpleThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumDarkPurpleThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-dark-purple", XO("Audacium Dark Purple") }, { ImageCacheAsData, false /* is default */}
};
