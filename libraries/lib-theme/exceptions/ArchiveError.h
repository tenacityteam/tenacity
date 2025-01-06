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
            Invalid,                 /// Invalid ZIP or JSON data.
            OperationalError,        /// libzip or jsoncpp error.
            ResourceNotFound,        /// Resource not found.
            MissingRequiredResource, /// Required resource missing.
            MissingRequiredAttribute /// Required theme attribute missing.
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
