/*!********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file DarkThemeAsCeeCode.cpp
 
 Paul Licameli split from Theme.cpp
 
 **********************************************************************/

#include <vector>
#include "ThemeLegacy.h"

static const std::vector<unsigned char> ImageCacheAsData {
// Include the generated file full of numbers
#include "AudacityDarkThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
   { "audacity-dark", XO("Audacity Dark") }, PreferredSystemAppearance::Dark, ImageCacheAsData
};
