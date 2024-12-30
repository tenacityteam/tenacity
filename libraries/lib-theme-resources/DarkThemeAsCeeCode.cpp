/**********************************************************************
 
  Tenacity
 
  @file DarkThemeAsCeeCode.cpp
 
  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later
 
***********************************************************************/

#include <vector>
#include "Theme.h"

static const std::vector<unsigned char> ImageCacheAsData {
    // Include the generated file full of numbers
    #include "DarkThemeAsCeeCode.h"
};

static ThemeBase::RegisteredTheme theme{
    // i18n-hint: greater difference between foreground and background colors
    { "dark", XO("Dark") },
    PreferredSystemAppearance::Dark,
    ImageCacheAsData
};
