/**********************************************************************

  Tenacity

  Theme.h

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/
#pragma once

#include <any>
#include <list>
#include <unordered_map>
#include <string>

#include "Types.h"

class THEME_API Theme final
{
    public:
        Theme() = default;
        Theme(const Theme& other) : mResources{other.mResources} {}
        Theme(Theme&& other) : mResources{std::move(other.mResources)} {}

        Theme& operator=(const Theme& other);
        Theme& operator=(Theme&& other);

        /// @brief Returns the theme package's resurce map if available.
        const ThemeResourceMap& GetResourceMap();

        /** @brief Returns data associated with an individual resource.
         * 
         * @param name The name of the resource to get.
         * 
         * @return Returns data with the associated resource.
         * 
        */
        const std::any& GetResource(const std::string& name);

        /** @brief Adds a resource.
         * 
         * @param name The resource name to add.
         * @param data The data to associate with the new resource. Optional.
         * 
        */
       void AddResource(const std::string& name, const std::any data = {});

    private:
        ThemeResourceMap mResources;
};
