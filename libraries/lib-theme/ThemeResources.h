/**********************************************************************

  Tenacity

  ThemeResources.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <any>
#include <string>
#include <unordered_map>
#include <list>

/** @brief Manages theme resources such as colors and bitmaps.
 * 
 * Resources are associated with a string ID, and their data is represented
 * using `std::any`, allowing it to be dealt with in a toolkit neutral fashion.
 * It also means that any change in representation does not require
 * modification on lib-theme's end.
 * 
 * Theme resources can be "registered", although all you would be doing is
 * merely adding it.
 * 
 **/
class THEME_API ThemeResources
{
    public:
        using Map = std::unordered_map<std::string, std::any>;
        using List = std::list<std::string>;

    private:
        Map mResources;

        ThemeResources()  = default;
        ~ThemeResources() = default;

    public:
        static ThemeResources& Get();

        /** @brief Creates a new theme resource.
         * 
         * Note that this does not allow for setting an existing resource. See
         * @ref SetResource() to do that
         * 
         * @param name The name of the resource.
         * @param data The data of the resource, which can be literally
         * _anything_.
         * 
         **/
        void AddResource(const std::string& name, std::any& data);

        /** @brief Assigns data to an existing resource.
         * 
         * Note that this does not allow for creating an existing resource. See
         * @ref AddResource() to do that.
         * 
         * @throws std::invalid_argument The resource doesn't exist.
         * 
         */
        void SetResource(const std::string& name, std::any& data);

        /** @brief Retrieves data associated with a resource.
         *
         * @throws std::invalid_argument The resource doesn't exist.
         * 
         * @param name The resource name to get the associated data from.
         * @return Returns the data associated with a resource.
         **/
        const std::any& GetResourceData(const std::string& name) const;

        /** @brief Checks if a resource exists.
         * 
         * @param name The name of the resource to check if it exists.
         * @returns True if the resource exists, false otherwise.
         * 
         **/
        bool CheckIfExists(const std::string& name) const;

        /// Creates a list of all existing theme resource IDs on-demand.
        List GetKnownResourceNames() const;

        /** @brief Clears all resources.
         * 
         * Clears all known resource names and their associated data.
         * 
         * @warning If any manually-allocated resources are stored, it is your
         * responsibility that they are properly deallocated.
         * 
         */
        void ClearAll();

        /// Returns true if there are any existing theme resources
        bool ContainsResources() const noexcept;
};
