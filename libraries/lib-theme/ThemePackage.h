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
#include <vector>
#include <fstream>
#include <sstream>

// Undefine new because jsoncpp uses in-place initialization for
// SecureAllocator
#ifdef _CRTDBG_MAP_ALLOC
#undef new
#endif

#include <json/value.h>
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
        Json::Value mInfo;
        Json::Value mColors;
        Json::Value mCurrentSubthemeInfo;
        std::string mSelectedSubtheme;
        bool mIsMultiThemePackage;

        /// @brief Reads a file from the package archive and returns a buffer
        /// containing all the file's contents.
        /// @param name The name of the file to read from the archive.
        /// @return Returns an allocated buffer of all the file's contents.
        std::unique_ptr<char> ReadFileFromArchive(const std::string& name);

        /** @brief Returns a parsed JSON file.
         * 
         * This member function reads an entire JSON file in memory and puts it
         * in a string stream. It then parses the stream and returns it.
         * 
         * @param jsonFile The JSON file to parse.
         * 
         * @return Returns a parsed JSON data.
         * 
         **/
        Json::Value GetParsedJsonData(const std::string& jsonFile);

    public:
        ThemePackage();
        ~ThemePackage();

        // ThemePackage can only be moved, not copied.
        ThemePackage(const ThemePackage&) = delete;
        ThemePackage& operator=(const ThemePackage&) = delete;

        ThemePackage(ThemePackage&& other);
        ThemePackage& operator=(ThemePackage&& other);

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

        /** @brief Check if the package is in a valid state.
         * 
         * A package is in a valid state if:
         * 
         * 1. The package was successfully loaded (see
         *    @ref SuccessfullyLoaded()).
         * 2. For multi-theme packages, the "subthemes" property is an array.
         * 
        */
        bool IsValid() const;

        /** @brief Checks if the theme package was successfully loaded.
         * 
         * This member function only checks if the package was successfully
         * loaded. A package might have successfully loaded but not be in a
         * valid state. For example, it may load successfully but might have
         * the "subthemes" property in it's root info.json incorrectly defined.
         * 
         * A package is considered successfully loaded if:
         * 
         * 1. It was opened.
         * 2. The archive contains no errors.
         * 3. info.json and colors.json have been parsed.
         * 
        */
        bool SuccessfullyLoaded() const;

        /** @brief Loads a subtheme.
         * 
         * This member function loads a subtheme based on `theme`. If `theme` is
         * empty, it loads the theme at the root of the pakcage (i.e., a
         * single-theme package). If the theme in `theme` doesn't exist, a
         * NonExistentTheme exception is thronw. If the package is a
         * single-theme package, this function ignores `theme` and loads the
         * current theme.
         * 
         * @param theme The theme to load. This should be a path relative to
         * the root of the archive. By default, it loads the theme from the
         * root.
         * 
        */
        void LoadTheme(const std::string& theme = {});

        /** @brief Loads an entire resource into memory.
         * 
         * Depending on the found resource, this can either be a `int` (for
         * colors) or a `std::vector<char>` (for files). Other resource types
         * may return different types. If the package is a multi-theme package,
         * resources are loaded from the currently selected subtheme.
         * 
         * If loading a resource fails, or if the resource isn't found, an
         * exception is thrown. This member function guarantees that a valid
         * value for std::any is returned.
         * 
         * @exception ArchiveError Thrown when reading from the archive failed
         * or when a resource wasn't found.
         * 
         * @param name The name of the resource to load. This should NOT be a
         * path relative to the archive.
         * 
         * @return Returns either an int (for color values) or a
         * std::vector<char> containing all of the resource's data.
         * 
        */
        std::any LoadResource(const std::string& name);

        /** @brief Returns an attribute from the package's info.json.
         * 
         * @param name The name of the attribute to get.
         * @return Returns the value of the attribute. Or Json::Value::null if
         * it doesn't exist.
         * 
        */
        Json::Value GetAttribute(const std::string& name);

        /// @brief Returns if the theme package contains multiple themes.
        bool IsMultiThemePackage() const;
};
