/**********************************************************************

  Tenacity

  ThemePackage.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ThemePackage.h"

#include <cassert>
#include <stdexcept>

#include <json/reader.h>

#include "exceptions/ArchiveError.h"
#include "exceptions/IncompatibleTheme.h"
#include "exceptions/InvalidState.h"

using namespace ThemeExceptions;

#define THROW_NOT_IMPLEMENTED throw std::runtime_error("Not implemented")

ThemePackage::ThemePackage()
: mPackageArchive{nullptr},
  mInfo{Json::Value::nullSingleton()},
  mColors{Json::Value::nullSingleton()}
{
}

ThemePackage& ThemePackage::operator=(ThemePackage&& other)
{
    mPackageArchive = other.mPackageArchive;
    other.mPackageArchive = nullptr;

    mInfo = std::move(other.mInfo);
    mColors = std::move(other.mColors);

    return *this;
}

ThemePackage::~ThemePackage()
{
    ClosePackage();
}

ThemePackage::ThemePackage(ThemePackage&& other)
{
    if (other.mPackageArchive)
    {
        mPackageArchive = other.mPackageArchive;
        other.mPackageArchive = nullptr;
    }

    mInfo = std::move(other.mInfo);
    mColors = std::move(other.mColors);
}

std::unique_ptr<char> ThemePackage::ReadFileFromArchive(const std::string& name)
{
    std::unique_ptr<char> fileData;
    zip_file_t* file;
    zip_stat_t fileInfo;
    zip_int64_t bytesRead;
    int error;

    // Get the file's size
    error = zip_stat(mPackageArchive, name.c_str(), ZIP_STAT_SIZE, &fileInfo);
    if (error != 0)
    {
        return nullptr;
    }

    // Allocate our data buffer to the file's size
    fileData.reset(new char[fileInfo.size]);

    // Open the file
    file = zip_fopen(mPackageArchive, name.c_str(), 0);
    if (!file) 
    {
        zip_fclose(file);
        return nullptr;
    }

    // Read the file
    bytesRead = zip_fread(file, fileData.get(), fileInfo.size);
    if (bytesRead != fileInfo.size)
    {
        zip_fclose(file);
        return nullptr;
    }

    zip_fclose(file);
    return fileData;
}

void ThemePackage::OpenPackage(const std::string& path)
{
    // Read the archive
    int error = 0;
    mPackageArchive = zip_open(path.c_str(), ZIP_RDONLY, &error);
    if (!mPackageArchive)
    {
        switch (error)
        {
            case ZIP_ER_INCONS:
            case ZIP_ER_READ:
            case ZIP_ER_SEEK:
                // TODO: better error handling
                throw ArchiveError(ArchiveError::Type::OperationalError);
                break;
            case ZIP_ER_NOZIP:
            case ZIP_ER_INVAL:
                throw ArchiveError(ArchiveError::Type::InvalidArchive);
                break;
            case ZIP_ER_MEMORY:
                throw std::bad_alloc();
                break;
        }
    }

    error = 0;

    // Read info.json from the archive all into memory.
    std::unique_ptr<char> data = ReadFileFromArchive("info.json");
    if (!data)
    {
        // TODO: Better error handling
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    std::istringstream jsonStream = std::istringstream(std::string(data.get()));
    {
        Json::CharReaderBuilder builder;
        std::string parserErrors;
        bool ok = Json::parseFromStream(builder, jsonStream, &mInfo, &parserErrors);
        if (!ok)
        {
            throw ArchiveError(ArchiveError::Type::OperationalError);
        }
    }

    // Read colors.json from the archive all into memory.
    data = ReadFileFromArchive("colors.json");
    if (!data)
    {
        throw ArchiveError(ArchiveError::Type::MissingRequiredResource);
    }

    jsonStream = std::istringstream(std::string(data.get()));
    {
        Json::CharReaderBuilder builder;
        std::string parserErrors;
        bool ok = Json::parseFromStream(builder, jsonStream, &mColors, &parserErrors);
        if (!ok)
        {
            throw ArchiveError(ArchiveError::Type::OperationalError);
        }
    }

    // Check for the images/ subdir
    zip_stat_t imageDir;

    error = zip_stat(mPackageArchive, "images/", ZIP_STAT_NAME, &imageDir);

    // If this fails, chances are it doesn't exist. Throw a missing required
    // resource error.
    if (error != 0)
    {
        throw ArchiveError(ArchiveError::Type::MissingRequiredResource);
    }
}

/** @brief Parses a version string.
 * 
 * This function works 
 * 
 * @param versionString The version string to parse
 * @return Returns a std::vector<int> containing the version string values.
 * 
*/
std::vector<int> ParseVersionString(const std::string& versionString)
{
    std::vector<int> version;
    std::string      tempString;
    int              tempVersion;
    std::size_t      previousPeriod = 0;
    std::size_t      period;

    // This is a very simple version string parsing algorithm that merely
    // creates substrings and converts them to an integer.
    do
    {
        period = versionString.find('.', previousPeriod);
        tempString = versionString.substr(previousPeriod, period - previousPeriod);

        try
        {
            tempVersion = std::stoi(tempString);
        } catch(...)
        {
            tempVersion = 0;
        }

        version.push_back(tempVersion);
        previousPeriod = period + 1;
    } while (period != std::string::npos);

    return version;
}

void ThemePackage::ParsePackage()
{
    const Json::Value themeName = mInfo["name"];
    Json::Value minAppVersionString = mInfo.get("minAppVersion", "0.0.0");
    std::vector<int> minAppVersion = ParseVersionString(minAppVersionString.asString());
    int minVersionMajor    = TENACITY_VERSION;
    int minVersionRelease  = TENACITY_RELEASE;
    int minVersionRevision = TENACITY_REVISION;

    // Handle theme name
    if (themeName.asString().empty())
    {
        // TODO: Better exception handling
        throw ArchiveError(ArchiveError::Type::MissingRequiredAttribute);
    }

    try
    {
        minVersionMajor = minAppVersion.at(0);
        minVersionRelease = minAppVersion.at(1);
        if (minAppVersion.size() >= 3) minVersionRevision = minAppVersion[2];
    } catch (...)
    {
        // Something happened when parsing the version number. Assume '0.0.0' by default
        minVersionMajor = minVersionRelease = minVersionRevision = 0;
    }

    // Handle minimum version compatibility
    if (minVersionMajor < TENACITY_VERSION || minVersionRelease < TENACITY_RELEASE)
    {
        // TODO: Better exception handling
        throw IncompatibleTheme(minVersionMajor, minVersionRelease, minVersionRevision);
    }

    // FIXME: Should the revision number really matter between theme packages?
    // I don't think it should, but I'm leaving this in until we decide on that
    // behavior...
    // else if (minVersionRevision < TENACITY_REVISION)
    // {
    //     // TODO: Better exception handling
    //     throw std::runtime_error("Incompatible theme");
    // }

    // TODO: handle other properities.
}

void ThemePackage::ClosePackage()
{
    if (mPackageArchive)
    {
        zip_close(mPackageArchive);
    }

    mPackageArchive = nullptr;
}

bool ThemePackage::IsValid() const
{
    if (!mPackageArchive) return false;

    // Check the archive for errors
    zip_error_t* err = zip_get_error(mPackageArchive);
    int zipError = zip_error_code_zip(err);
    int sysError = zip_error_code_system(err);

    if (zipError != ZIP_ER_OK || sysError != 0)
    {
        return false;
    }

    // Check the JSON values for any errors
    if (!mInfo || !mColors)
    {
        return false;
    }

    return true;
}

std::any ThemePackage::LoadResource(const std::string& name)
{
    if (!IsValid()) throw InvalidState();

    std::string currentResourceName;
    int status;
    std::any resourceData;

    // Search colors.json for any name matches first.
    int colorData;
    const Json::Value colorResource = mColors[name];
    if (colorResource != Json::Value::nullSingleton())
    {
        resourceData = colorResource.asInt();
        return resourceData;
    }

    // If no resources matched in colors.json, find it in images/
    std::vector<char> data;
    zip_int64_t numEntries = zip_get_num_entries(mPackageArchive, ZIP_FL_UNCHANGED);
    zip_stat_t currentFile;

    for (zip_int64_t i = 0; i < numEntries; i++)
    {
        if (zip_stat_index(mPackageArchive, i, ZIP_STAT_NAME | ZIP_STAT_SIZE, &currentFile) != 0)
        {
            throw ArchiveError(ArchiveError::Type::OperationalError);
        }

        // Special case: remove any preceding directories from the name in case
        // it's under any directory.
        currentResourceName = currentFile.name;

        size_t slash = currentResourceName.rfind("/");
        if (slash != std::string::npos)
        {
            if (slash < currentResourceName.size())
            {
                slash++;
            }

            if (slash != currentResourceName.size())
            {
                currentResourceName = currentResourceName.substr(slash);
            }
        }

        // Remove the file extension from the file name
        size_t dot = currentResourceName.rfind(".");
        if (dot != std::string::npos)
        {
            if (dot != currentResourceName.size())
            {
                currentResourceName = currentResourceName.substr(0, dot);
            }
        }

        // Read the entire file into memory
        if (currentResourceName == name)
        {
            // Use the original file name
            auto data = ReadFileFromArchive(currentFile.name);
            std::vector<char> buffer(data.get(), data.get() + currentFile.size);
            if (buffer.size() == 0)
            {
                // Reading failed. Throw an operational error
                throw ArchiveError(ArchiveError::Type::OperationalError);
            }

            resourceData = buffer;
        }
    }

    // resourceData should contain a value. If there was an error, throw an
    // exception since the resource wasn't found.
    if (!resourceData.has_value())
    {
        throw ArchiveError(ArchiveError::Type::ResourceNotFound);
    }

    return resourceData;
}

Json::Value ThemePackage::GetAttribute(const std::string& name)
{
    return mInfo.get(name, Json::Value::nullSingleton());
}

bool ThemePackage::IsMultiThemePackage() const
{
    if (!IsValid()) throw InvalidState();

    // FIXME: Unimplemented.
    return false;
}
