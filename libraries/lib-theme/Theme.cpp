/**********************************************************************

  Tenacity

  Theme.cpp

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#include "Theme.h"
#include "exceptions/ArchiveError.h"
#include "exceptions/InvalidState.h"
using namespace ThemeExceptions;

#include <utility>

Theme::Theme(ThemePackage&& package)
{
    mPackage = std::forward<ThemePackage>(package);
}

Theme& Theme::operator=(const Theme& other)
{
    mResources = other.mResources;
    return *this;
}

Theme& Theme::operator=(Theme&& other)
{
    mResources = std::move(other.mResources);
    mPackage = std::move(other.mPackage);
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

void Theme::SetPackage(ThemePackage&& package)
{
    mPackage = std::forward<ThemePackage>(package);
}

const ThemePackage& Theme::GetPackage()
{
    if (!mPackage)
    {
        throw InvalidState();
    }

    return mPackage.value();
}

void Theme::LoadAttributesFromPackage()
{
    if (!mPackage)
    {
        throw InvalidState();
    }

    // Load theme name
    auto value = mPackage->GetAttribute("name");
    mName = value.asString();
}

ThemePackage&& Theme::ReleasePackage()
{
    return std::move(mPackage.value());
}

const std::any& Theme::GetResource(const std::string& name)
{
    // First, search our map.
    auto exists = mResources.find(name);
    if (exists == mResources.end())
    {
        // The resource doesn't exist in our resource map, so load it from the
        // backing package if it exists.
        if (mPackage)
        {
            ThemePackage& package = mPackage.value();
            try
            {
                auto resource = package.LoadResource(name);
                mResources[name] = resource;
            } catch(ArchiveError& ae)
            {
                // Rethrow the exception.
                throw;
            }
        } else
        {
            throw ArchiveError(ArchiveError::Type::ResourceNotFound);
        }
    }

    // At this point, we should've added the resource to our resource map, so
    // go ahead and retrieve it from the map.
    return mResources.at(name);
}

void Theme::AddResource(const std::string& name, const std::any& data)
{
    mResources[name] = data;
}
