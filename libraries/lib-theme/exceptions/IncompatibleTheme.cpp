/**********************************************************************

  Tenacity

  IncompatbileTheme.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

// Paired with IncompatibleTheme.h solely to have matching source files
#include "IncompatibleTheme.h"

namespace ThemeExceptions
{

int IncompatibleTheme::GetMajor() const
{
    return mMinMajor;
}

int IncompatibleTheme::GetRelease() const
{
    return mMinRelease;
}

int IncompatibleTheme::GetRevision() const
{
    return mMinRevision;
}

} // namespace ThemeExceptions
