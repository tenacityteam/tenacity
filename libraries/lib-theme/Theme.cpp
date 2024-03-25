/**********************************************************************

  Tenacity

  Theme.cpp

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#include "Theme.h"

Theme& Theme::operator=(const Theme& other)
{
    mResources = other.mResources;
    return *this;
}

Theme& Theme::operator=(Theme&& other)
{
    mResources = std::move(other.mResources);
    return *this;
}

const ThemeResourceMap& Theme::GetResourceMap()
{
    return mResources;
}

const std::any& Theme::GetResource(const std::string& name)
{
    return mResources.at(name);
}
