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
 * Resources are associated with an ID (as a string). The resources themselves
 * can also be anything, which allows the class to be toolkit neutral in turn.
 * 
 * Theme resources can be registered, although registration in Tenacity is
 * taken "less seriously". That is, a component does not need to be registered
 * in order to be used in Tenacity.
 * 
 **/
class ThemeResources
{
    public:
        using ResourceMap = std::unordered_map<std::string, std::any>;
        using ResourceList = std::list<std::string>;

    private:
        ResourceMap mResources;

        ThemeResources()  = default;
        ~ThemeResources() = default;

    public:
        static ThemeResources& Get();

        /** @brief Adds a theme resource, creating it if it doesn't exist.
         * 
         * @param name The name of the resource.
         * @param data The data of the resource, which can be literally
         * _anything_.
         * 
         **/
        void AddResource(const std::string& name, std::any& data);

        /** @brief Retrieves data associated with a resource.
         *
         * @throws std::invalid_argument The resource doesn't exist.
         * 
         * @param name The resource name to get the associated data from.
         * @return Returns the data associated with a resource.
         **/
        std::any& GetResourceData(const std::string& name);

        /** @brief Checks if a resource exists.
         * 
         * @param name The name of the resource to check if it exists.
         * @returns True if the resource exists, false otherwise.
         * 
         **/
        bool CheckIfExists(const std::string& name) const;

        /// Creates a list of all existing theme resource IDs on-demand.
        ResourceList GetRegisteredResourceNames() const;
};
