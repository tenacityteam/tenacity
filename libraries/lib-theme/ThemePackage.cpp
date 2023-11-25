/**********************************************************************

  Tenacity

  ThemePackage.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ThemePackage.h"

#include <stdexcept>

#include <json/value.h>
#include <json/reader.h>

#include "exceptions/ArchiveError.h"
#include "exceptions/IncompatibleTheme.h"

using namespace ThemeExceptions;

#define THROW_NOT_IMPLEMENTED throw std::runtime_error("Not implemented")

ThemePackage::ThemePackage() : mPackageArchive{nullptr}
{
}

ThemePackage::~ThemePackage()
{
    ClosePackage();
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

    // Extract the JSON data from the archive
    std::unique_ptr<char> jsonData;
    zip_stat_t jsonInfo;

    error = zip_stat(mPackageArchive, "theme.json", ZIP_STAT_SIZE, &jsonInfo);
    if (error != 0)
    {
        // TODO: Better error handling
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    // Read the entire theme.json into memory
    jsonData.reset(new char[jsonInfo.size]);
    zip_file_t* themeFile = zip_fopen(mPackageArchive, "theme.json", 0);
    zip_int64_t bytesRead = zip_fread(themeFile, jsonData.get(), jsonInfo.size);
    if (bytesRead != jsonInfo.size)
    {
        zip_fclose(themeFile);
        // TODO: Better error handling
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    std::string jsonString = jsonData.get();
    mJsonStream = std::istringstream(jsonString);
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
    if (mJsonStream.str().empty())
    {
        return;
    }

    Json::Value packageRoot;
    {
        Json::CharReaderBuilder builder;
        std::string parserErrors;
        bool ok = Json::parseFromStream(builder, mJsonStream, &packageRoot, &parserErrors);
        if (!ok)
        {
            throw ArchiveError(ArchiveError::Type::OperationalError);
        }
    }

    // Check if the theme package is a multi-theme package. If so, parse those separately
    const Json::Value subthemes = packageRoot["subthemes"];
    if (subthemes)
    {
        // TODO: handles subthemes
        // throw std::runtime_error("Not implemented yet!");
        return;
    }

    const Json::Value themeName = packageRoot["name"];
    Json::Value minAppVersionString = packageRoot.get("minAppVersion", "0.0.0");
    std::vector<int> minAppVersion = ParseVersionString(minAppVersionString.asString());
    int minVersionMajor    = TENACITY_VERSION;
    int minVersionRelease  = TENACITY_RELEASE;
    int minVersionRevision = TENACITY_REVISION;

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

    // Handle theme name
    if (themeName.asString().empty())
    {
        // TODO: Better exception handling
        throw std::runtime_error("Theme package does not have a name!");
    }

    // TODO: handle other properities.
}

void ThemePackage::ClosePackage()
{
    if (mPackageArchive)
    {
        zip_close(mPackageArchive);
    }
}

ThemePackage::ResourceMap& ThemePackage::GetResourceMap()
{
    return mThemeResources;
}

void ThemePackage::LoadAllResources()
{
    THROW_NOT_IMPLEMENTED;
}

void ThemePackage::LoadResource(const std::string name)
{
    THROW_NOT_IMPLEMENTED;
}

void ThemePackage::LoadResources(const ThemePackage::ResourceList& names)
{
    THROW_NOT_IMPLEMENTED;
}

std::any& ThemePackage::GetResource(std::string name)
{
    return mThemeResources.at(name);
}

std::string ThemePackage::GetName() const
{
    return mPackageName;
}

bool ThemePackage::IsMultiThemePackage() const
{
    // FIXME: Unimplemented.
    return false;
}
