/**********************************************************************

  Tenacity

  InvalidState.h

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/
#pragma once

namespace ThemeExceptions
{

/// Thrown on an attempt to perform an operation on a Theme or ThemePackage
/// when it's in an invalid state.
class InvalidState
{
    public:
        InvalidState() = default;
        ~InvalidState() = default;
};

} // namespace ThemeExceptions
