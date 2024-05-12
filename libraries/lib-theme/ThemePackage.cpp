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
  mColors{Json::Value::nullSingleton()},
  mSelectedSubtheme{""},
  mIsMultiThemePackage{false}
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

Json::Value ThemePackage::GetParsedJsonData(const std::string& jsonFile)
{
    std::unique_ptr<char> data = ReadFileFromArchive(jsonFile);
    if (!data)
    {
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    std::istringstream jsonStream = std::istringstream(std::string(data.get()));

    Json::CharReaderBuilder builder;
    Json::Value value;
    std::string parserErrors;
    bool ok = Json::parseFromStream(builder, jsonStream, &value, &parserErrors);
    if (!ok)
    {
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    return value;
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
                throw ArchiveError(ArchiveError::Type::Invalid);
                break;
            case ZIP_ER_MEMORY:
                throw std::bad_alloc();
                break;
        }
    }

    error = 0;

    // Read info.json from the archive all into memory.
    mInfo = GetParsedJsonData("info.json");

    // Check if the theme package contains a "subthemes" element. If it does, it
    // contains multiple subthemes.
    if (mInfo.isMember("subthemes") && mInfo["subthemes"].isArray())
    {
        mIsMultiThemePackage = true;

        auto subtheme = mInfo["subthemes"][0];

        if (subtheme.isString())
        {
            mSelectedSubtheme = subtheme.asString() + "/";
            mIsMultiThemePackage = true;
        }

        // Don't parse the rest of the package. A compliant theme package will
        // not have them at the root of the archive.
        return;
    }
}

/** @brief Parses a version string.
 * 
 * This function expects a version string in the 'x.y.z' format and only
 * parses the major, minor, and patch releases. Anything else is not parsed. If
 * a version number can't be parsed, it defaults to 0.
 * 
 * @param versionString The version string to parse.
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
    // Prepare the package first.
    LoadTheme(mSelectedSubtheme);

    Json::Value themeName;
    Json::Value minAppVersionString;
    std::vector<int> minAppVersion;
    int minVersionMajor    = TENACITY_VERSION;
    int minVersionRelease  = TENACITY_RELEASE;
    int minVersionRevision = TENACITY_REVISION;

    try
    {
        auto themeInfo = IsMultiThemePackage() ? mCurrentSubthemeInfo : mInfo;

        if (!themeInfo.isMember("name"))
        {
            throw ArchiveError(ArchiveError::Type::Invalid);
        }

        themeName = themeInfo["name"];
        minAppVersionString = themeInfo.get("minAppVersion", "0.0.0");
        minAppVersion = ParseVersionString(minAppVersionString.asString());
    } catch (Json::LogicError&)
    {
        throw ArchiveError(ArchiveError::Type::Invalid);
    }

    // Handle theme name
    if (themeName.asString().empty())
    {
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

    mInfo = Json::Value::nullSingleton();
    mColors = Json::Value::nullSingleton();
    mCurrentSubthemeInfo = Json::Value::nullSingleton();
    mSelectedSubtheme.clear();
}

bool ThemePackage::IsValid() const
{
    if (!SuccessfullyLoaded())
    {
        return false;
    }

    // Check if "subthemes", if a multi-theme package, is an array
    if (mIsMultiThemePackage && !mInfo["subthemes"].isArray())
    {
        return false;
    }

    return true;
}

bool ThemePackage::SuccessfullyLoaded() const
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

void ThemePackage::LoadTheme(const std::string& theme)
{
    // Parse the subtheme's info.json into memory, but only if we're dealing
    // with a multi-theme package.
    if (mIsMultiThemePackage)
    {
        mCurrentSubthemeInfo = GetParsedJsonData(theme + "info.json");
    }

    // Parse colors.json from the archive all into memory.
    mColors = GetParsedJsonData(theme + "colors.json");

    // Check for the images/ subdir
    zip_stat_t imageDir;

    std::string imagesPath = theme + "images/";
    int error = zip_stat(mPackageArchive, imagesPath.c_str(), ZIP_STAT_NAME, &imageDir);

    // If this fails, chances are it doesn't exist. Throw a missing required
    // resource error.
    if (error != 0)
    {
        throw ArchiveError(ArchiveError::Type::MissingRequiredResource);
    }
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

    return mIsMultiThemePackage;
}
