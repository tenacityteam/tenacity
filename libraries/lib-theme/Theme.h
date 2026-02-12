/**********************************************************************

  Tenacity

  Theme.h

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/
#pragma once

#include <any>
#include <list>
#include <optional>
#include <unordered_map>
#include <string>

#include "Types.h"

/** @brief Represents a theme in Tenacity.
 * 
 * This class is completely different from Audacity's Theme class (which
 * Tenacity inherited). It is rewritten to contain its own set of theme
 * resources rather than act as a singleton for managing themes. See @ref
 * ThemeResources to see that functionality.
 * 
 * Themes can be constructed either from a theme package or in-memory. The
 * @ref ThemePackage class allows you to load a theme from a theme package or
 * create one in meomry. Regardless of how a theme is created, themes do not
 * know whether they originate from a theme package or not.
 */
class THEME_API Theme final
{
    private:
        std::string mName;
        ThemeResourceMap mResources;

    public:
        Theme() = default;

        /** @brief Copy constructor
         *
         * @warning Calling one of these constructors means **everything** in
         * the theme is copied, including resources! You probably want to use a
         * reference to a Theme instead.
         */
        Theme(const Theme& other) : mResources{other.mResources} {}

        /// Move constructor.
        Theme(Theme&& other) : mResources{std::move(other.mResources)} {}

        /** @brief Copy assignment operators.
         *
         * @warning Calling one of these operators means **everything** in the
         * theme is copied, including resources! You probably want to use a
         * reference to a Theme instead.
         */
        Theme& operator=(const Theme& other);

        /// Move assignment operator.
        Theme& operator=(Theme&& other);

        /// Returns the theme's resource map if available.
        const ThemeResourceMap& GetResourceMap();

        void SetName(const std::string& name);
        std::string GetName();

        /** @brief Returns data associated with an individual resource.
         * 
         * If the resource doesn't exist in the map, it is automatically loaded
         * from the backing theme package if available. After it is loaded, it
         * is added to its resource map.
         * 
         * @param name The name of the resource to get.
         * 
         * @exception InvalidState Thrown if there is a valid backing theme
         * package but the package is invalid.
         * 
         * @exception ArchiveError Thrown if the resource wasn't found.
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
       void AddResource(const std::string& name, const std::any& data = {});
};
