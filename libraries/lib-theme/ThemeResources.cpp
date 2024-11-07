/**********************************************************************

  Tenacity

  ThemeResources.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ThemeResources.h"
#include <stdexcept>

ThemeResources& ThemeResources::Get()
{
    static ThemeResources theResources;
    return theResources;
}

void ThemeResources::AddResource(const std::string& name, std::any& data)
{
    mResources.emplace(name, data);
}

void ThemeResources::SetResource(const std::string& name, std::any& data)
{
    mResources[name] = data;
}

const std::any& ThemeResources::GetResourceData(const std::string& name) const
{
    return mResources.at(name);
}

bool ThemeResources::CheckIfExists(const std::string& name) const
{
    return mResources.find(name) != mResources.end();
}

ThemeResources::List ThemeResources::GetKnownResourceNames() const
{
    ThemeResources::List resourceNames;

    for (auto& resource : mResources)
    {
        resourceNames.push_back(resource.first);
    }

    return resourceNames;
}

void ThemeResources::ClearAll()
{
    mResources.clear();
}

bool ThemeResources::ContainsResources() const noexcept
{
    return !mResources.empty();
}
