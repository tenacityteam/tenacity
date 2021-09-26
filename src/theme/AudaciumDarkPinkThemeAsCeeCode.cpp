/*!********************************************************************

  Tenacity

  @file AudaciumDarkPinkThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudaciumDarkPinkThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacium-dark-pink", XO("Audacium Dark Pink") }, ImageCacheAsData
};
