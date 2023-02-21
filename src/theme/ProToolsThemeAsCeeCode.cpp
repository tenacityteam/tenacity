/*!********************************************************************

  Tenacity

  @file ProToolsThemeAsCeeCode.cpp

  Avery King split from Theme.cpp

  (This is copied from DarkThemeAsCeeCode.h; I didn't write anything new)

**********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "ProToolsThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "pro-tools", XO("Pro Tools") }, { ImageCacheAsData, false /* is default */}
};
