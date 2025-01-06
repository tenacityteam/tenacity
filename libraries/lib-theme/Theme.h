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

#include "ThemePackage.h"
#include "Types.h"

/** @brief Represents a theme in Tenacity.
 * 
 * This class is completely different from Audacity's Theme class (which
 * Tenacity inherited). It is rewritten to contain its own set of theme
 * resources rather than act as a singleton for managing themes. See @ref
 * ThemeResources to see that functionality.
 * 
 * Themes can be constructed either from a ThemePackage or in-memory. If a
 * theme is backed by a ThemePackage, it can commit any changes to the package.
 * (Note that it's currently not possible to create an in-memory ThemePackage.
 * See TODO for more details).
 * 
 * @todo Implementing the following:
 * 
 * 1. Support for multi-theme packages.
 * 2. Support for committing in-memory changes.
 * 
*/
class THEME_API Theme final
{
    private:
        std::optional<ThemePackage> mPackage;
        std::string mName;
        ThemeResourceMap mResources;

    public:
        Theme() = default;

        /// Copy constructor. Any backing theme package is **not** copied.
        Theme(const Theme& other) : mResources{other.mResources} {}
        Theme(Theme&& other) : mResources{std::move(other.mResources)} {}

        /// Constructs a theme from a ThemePackage
        Theme(ThemePackage&& package);

        /// Copy assignment operator. Any backing theme package is **not**
        /// copied.
        Theme& operator=(const Theme& other);
        Theme& operator=(Theme&& other);

        /// Returns the theme package's resurce map if available.
        const ThemeResourceMap& GetResourceMap();

        void SetName(const std::string& name);
        std::string GetName();

        /** @brief Sets the backing theme package.
         * 
         * Any current backing package is closed after a call to this function.
         * To unset the backing theme package, call @ref ReleasePackage()
         * ignoring the return value.
         * 
         * Because ThemePackage cannot be copied, it must be moved, and thus
         * Theme takes ownership over the package. You can still access the
         * package via @ref GetPackage().
         * 
         * @param package The package to set.
         * 
        */
        void SetPackage(ThemePackage&& package);

        /// Returns the backing theme package.
        const ThemePackage& GetPackage();

        /** @brief Loads all attributes from a backing theme package.
         * 
         * @exception InvalidState Thrown if there is no backing package set.
         * 
        */
        void LoadAttributesFromPackage();

        /** @brief Releases the current theme package if any.
         * 
         * This member functions releases ownership of the current theme
         * package to the current owner. Simply ignore the return value if you
         * only want to reset the backing package.
         * 
        */
        ThemePackage&& ReleasePackage();

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
