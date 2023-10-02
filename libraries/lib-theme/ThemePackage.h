/**********************************************************************

  Tenacity

  ThemePackage.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <any>
#include <list>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

#include <zip.h>

/** @brief Represents a theme package.
 * 
 * Theme packages are Tenacity are mere ZIP archives that have the following
 * files:
 * 
 * Root:
 *     colors.json
 *     bitmaps
 *         bmpRecord.png
 *         bmpXXXXXX.png
 * 
 * 'colors.json' defines all the colors in the theme. All color properties are
 * prefixed with 'clr' to indicate they're a color. On the other hand,
 * 'bitmaps/' contains all the images that the theme file provides. All bitmaps
 * in Tenacity are prefixed with 'bmp' and thus represent a 
 * 
 * Each instance of this class allows you to load a theme resource but does not
 * load any resource by default. Additionally, this class has no knowledge of
 * what is a recognized theme resource or not.
 * 
 **/
class ThemePackage final
{
    public:
        using ResourceMap  = std::unordered_map<std::string, std::any>;
        using ResourceList = std::list<std::string, std::any>;

    private:
        zip_t* mPackageArchive;
        std::ifstream mPackageStream;
        ResourceMap mThemeResources;
        std::istringstream mJsonStream;

        // Atributes //////////////////////////////////////////////////////////
        std::string mPackageName;

    public:
        ThemePackage();
        ~ThemePackage();

        /** @brief Opens a theme package.
         * 
         * OpenPackage() first extracts a package with libzip and then attempts
         * to parse it. If reading the archive fails, an exception will be
         * thrown.
         * 
        */
        void OpenPackage(const std::string& path);

        /** @brief Parses the package 
         * 
         * The package must have been opened prior to parsing. If the package
         * was not opened prior to calling this function, an exception is
         * thrown.
         * 
        */
        void ParsePackage();

        /** @brief Closes a theme package.
         * 
         * This member function closes the backing package file and clears all
         * loaded resources
         * 
        */
        void ClosePackage();

        /** @brief Returns the theme package's resurce map if available.
         * 
         * **It is possible that an empty or incomplete map is returned by this
         * function**. Call @ref LoadAllResources() to ensure the resource map is
         * fully loaded.
         * 
         * @return Returns the theme package's resource map in whatever stage
         * it is.
         * 
         **/
        ResourceMap& GetResourceMap();

        /** @brief Loads all resources in the theme package.
         * 
         * By default, no resources are loaded as ThemePackage does not know
         * what resources the application will need. The application should
         * take steps to ensure that it only loads the resources it knows it
         * will use.
         * 
         **/
        void LoadAllResources();

        /// @brief Preloads a single resource.
        /// @param name The name of the resource to load.
        void LoadResource(const std::string name);

        /// @brief Preloads multiple resources.
        /// @param names A list of resources to preload.
        void LoadResources(const ResourceList& names);

        /// @brief Returns associated data with the resource, loading it if it
        /// has not been already loaded.
        /// @param name The name of the resource to get the associated data
        /// from.
        /// @return Returns the data associated with the resource.
        std::any& GetResource(const std::string name);

        /// Returns the theme package's name.
        std::string GetName() const;

        /// @brief Returns if the theme package contains multiple themes.
        bool IsMultiThemePackage() const;
};
