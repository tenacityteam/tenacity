/**********************************************************************

  Tenacity

  ArchiveError.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

namespace ThemeExceptions
{

class ArchiveError final
{
    public:
        enum class Type
        {
            InvalidArchive,  // Invalid ZIP or theme.json
            OperationalError // libzip or jsoncpp error
        };

    public:
        ArchiveError() = default;
        ArchiveError(Type error) : mError(error) {}
        ~ArchiveError() = default;

        Type GetErrorType() const
        {
            return mError;
        }

    private:
        Type mError;
};

} // namespace ThemeExceptions
