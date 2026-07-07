/**********************************************************************

  Tenacity

  Types.h

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#pragma once

#include <any>
#include <list>
#include <string>
#include <unordered_map>

using ThemeResourceMap  = std::unordered_map<std::string, std::any>;
using ThemeResourceList = std::list<std::string, std::any>;

