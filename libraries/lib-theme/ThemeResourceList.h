/**********************************************************************

  Tenacity

  ThemePackage.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <list>
#include <string>

/** @brief A list of image theme resource names known to Tenacity.
 * 
 * This list does _not_ define the only supported image resource names in
 * Tenacity. Other resources known by different names can be loaded by
 * Tenacity. Rather, this list serves as an optimization so Tenacity doesn't
 * load every single theme resource from a theme package.
 * 
 * Also note the type of this list. We keep expected image sizes here because
 * 
 * 
*/
extern const std::list<
    std::tuple<
        std::string, // Image name
        size_t,      // Expected image width
        size_t       // Expected image height
    >
> KnownImageResourceNames;

/** @brief A list of color theme resource names known to Tenacity.
 * 
 * This list does _not_ define the only support color resource names in
 * Tenacity. Other resources known by different names can be loaded by
 * Tenacity. Rather, this list serves as an optimization so Tenacity doesn't
 * load every single theme resource from a theme package.
 * 
 */
extern const std::list<std::string> KnownColorResourceNames;

