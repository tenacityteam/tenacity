/**********************************************************************

  Tenacity

  IncompatbileTheme.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

namespace ThemeExceptions
{

class IncompatibleTheme
{
    private:
        int mMinMajor;
        int mMinRelease;
        int mMinRevision;

    public:
        IncompatibleTheme() = default;
        IncompatibleTheme(int major, int release, int revision)
        : mMinMajor{major}, mMinRelease{release}, mMinRevision{revision} {}
        ~IncompatibleTheme() = default;

        int GetMajor() const;
        int GetRelease() const;
        int GetRevision() const;
};

} // namespace ThemeExceptions
