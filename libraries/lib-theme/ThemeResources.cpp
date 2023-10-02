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
    mResources.insert({name, data});
}

std::any& ThemeResources::GetResourceData(const std::string& name)
{
    return mResources.at(name);
}

bool ThemeResources::CheckIfExists(const std::string& name) const
{
    try
    {
        mResources.at(name);
    } catch (std::out_of_range&)
    {
        return false;
    }

    return true;
}

ThemeResources::ResourceList ThemeResources::GetRegisteredResourceNames() const
{
    ThemeResources::ResourceList resourceNames;

    for (auto& resource : mResources)
    {
        resourceNames.push_back(resource.first);
    }

    return resourceNames;
}
