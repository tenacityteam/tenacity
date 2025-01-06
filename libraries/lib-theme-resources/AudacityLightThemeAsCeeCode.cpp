/*!********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file AudacityLightThemeAsCeeCode.cpp
 
 Paul Licameli split from Theme.cpp
 
 **********************************************************************/

#include <vector>
#include "ThemeLegacy.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudacityLightThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   /* i18n-hint: Light meaning opposite of dark */
   { "audacity-light", XO("Audacity Light") }, PreferredSystemAppearance::Light, ImageCacheAsData
};
