/**********************************************************************

  Tenacity

  ThemePackage.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <any>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

#include <zip.h>

#include "Types.h"

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
class THEME_API ThemePackage final
{
    private:
        zip_t* mPackageArchive;
        std::ifstream mPackageStream;
        std::istringstream mJsonStream;
        std::istringstream mColorsStream;

        // Atributes //////////////////////////////////////////////////////////
        std::string mPackageName;

        /// @brief Reads a file from the package archive and returns a buffer
        /// containing all the file's contents.
        /// @param name The name of the file to read from the archive.
        /// @return Returns an allocated buffer of all the file's contents.
        std::unique_ptr<char> ReadFileFromArchive(const std::string& name);

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
        void LoadResources(const ThemeResourceList& names);

        /// Returns the theme package's name.
        std::string GetName() const;

        /// @brief Returns if the theme package contains multiple themes.
        bool IsMultiThemePackage() const;
};
