/**********************************************************************

  Tenacity

  ThemePackage.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <list>
#include <string>

/** @brief A list of theme resource names known to Tenacity.
 * 
 * This list does _not_ define the only supported resource names in Tenacity.
 * Other resources known by different names can be loaded by Tenacity. Rather,
 * this list serves as a list of known theme resources.
 * 
*/
extern const std::list<std::string> KnownThemeResourceNames;
