/*!********************************************************************

  Tenacity

  @file SaucedacityThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything new)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "SaucedacityThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "saucedacity", XO("Saucedacity") }, { ImageCacheAsData, false /* is default */}
};
