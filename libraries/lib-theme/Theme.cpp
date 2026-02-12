/**********************************************************************

  Tenacity

  Theme.cpp

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#include "Theme.h"
#include "exceptions/ArchiveError.h"
using namespace ThemeExceptions;

#include <utility>

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

void Theme::SetName(const std::string& name)
{
    mName = name;
}

std::string Theme::GetName()
{
    return mName;
}

const std::any& Theme::GetResource(const std::string& name)
{
    // First, search our map.
    auto exists = mResources.find(name);
    if (exists == mResources.end())
    {
        // TODO: Find better exception to throw than ArchiveError
        throw ArchiveError(ArchiveError::Type::ResourceNotFound);
    }

    // At this point, we should've added the resource to our resource map, so
    // go ahead and retrieve it from the map.
    return mResources.at(name);
}

void Theme::AddResource(const std::string& name, const std::any& data)
{
    mResources[name] = data;
}
