/**********************************************************************

  Tenacity

  ThemePackage.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <any>
#include <string>
#include <unordered_map>
#include <vector>

#include <simdjson.h>
#include <zip.h>

#include "Theme.h"

using namespace simdjson;

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

        /// The JSON parser. We use simdjson's DOM API due to issues with the
        /// on-demand API (we get incorrect type errors for some reason). 
        dom::parser mParser;

        /// The name of the theme (single-theme packages only).
        std::string_view mName;

        /// Contains paths to different subthemes in the package (multi-theme
        /// packages only).
        std::vector<std::string_view> mSubthemes;

        /// Contains all color names and their values from `colors.json` at the
        /// root of the archive (this is all the theme's colors for a
        /// single-theme package).
        std::unordered_map<std::string_view, std::string_view> mColors;

        /// Whether or not the theme package contains multiple themes.
        bool mIsMultiThemePackage;

        /// @brief Reads a file from the package archive and returns a buffer
        /// containing all the file's contents.
        /// @param name The name of the file to read from the archive.
        /// @return Returns a std::vector<char> of all the file's contents.
        std::vector<char> ReadFileFromArchive(const std::string& name);

        /** @brief Returns a parsed JSON file.
         * 
         * This member function reads an entire JSON file in memory and puts it
         * in a string stream. It then parses the stream and returns it.
         * 
         * @param jsonFile The JSON file to parse.
         * 
         * @return Returns a simdjson::dom::element object containing the
         * parsed data.
         * 
         **/
        dom::element GetParsedJsonData(const std::string& jsonFile);

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
         * OpenPackage() first extracts a package with libzip. Then, it parses
         * all of `info.json`, first checking the package's minimum supported
         * version. If that fails, @ref IncompatibleTheme is thrown.
         *
         * Next, all subtheme paths are collected from `info.json` if such are
         * found. Otherwise, the theme's name is collected. Finally, all colors
         * are parsed. (If the package is a multi-theme package, then only the
         * base colors at the root of the archive are parsed if they exist).
         *
         * On failure, an exception is thrown and the package is closed.
         *
         * @param path The path of the package to open.
         *
         * @exception ArchiveError Occurs if either opening the archive failed
         * or a parsing error occurred (likely invalid JSON).
         *
         * @exception IncompatibleTheme Thrown if the package requires a newer
         * version of Tenacity than the current one.
         * 
         */
        void OpenPackage(const std::string& path);

        /** @brief Closes a theme package.
         * 
         * This member function closes the backing package file and clears all
         * loaded resources.
         * 
        */
        void ClosePackage();

        /// Returns true if the theme package is currently open, false otherwise.
        bool IsOpen() const { return mPackageArchive; };

        /** @brief Gets all valid subthemes.
         *
         * This member function returns the paths of all subthemes. Each path
         * is relative to the root of the archive and may not necessarily
         * represent the theme's name. To get a subtheme's name, you must load
         * it via @ref LoadTheme.
         *
         * Use this member function to get a subtheme path to pass to @ref
         * LoadTheme.
         *
         * @return A string array containing all the subtheme paths, or an
         * empty array if the package is not a multi-theme package.
        */
        const std::vector<std::string_view>& GetSubthemes() const { return mSubthemes; }

        /** @brief Loads a subtheme.
         *
         * @todo Finish rewrite with LoadTheme().
         * 
         * This member function loads a subtheme based on `theme`. If `theme` is
         * empty, it loads the theme at the root of the package (i.e., a
         * single-theme package). If the theme in `theme` doesn't exist, a
         * NonExistentTheme exception is thrown. If the package is a
         * single-theme package, this function ignores `theme` and loads the
         * current theme.
         * 
         * @param theme The theme to load. This should be a path relative to
         * the root of the archive. By default, it loads the theme from the
         * root.
         * 
        */
        Theme LoadTheme(const std::string& theme = {});

        /// @brief Returns if the theme package contains multiple themes.
        bool IsMultiThemePackage() const;
};
