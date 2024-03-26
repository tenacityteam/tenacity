/**********************************************************************

  Tenacity

  ThemePackage.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ThemePackage.h"

#include <stdexcept>

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
        bool ok = Json::parseFromStream(builder, jsonStream, &mPackageRoot, &parserErrors);
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
    const Json::Value themeName = mPackageRoot["name"];
    Json::Value minAppVersionString = mPackageRoot.get("minAppVersion", "0.0.0");
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

void ThemePackage::LoadAllResources()
{
    THROW_NOT_IMPLEMENTED;
}

void ThemePackage::LoadResource(const std::string name)
{
    THROW_NOT_IMPLEMENTED;
}

void ThemePackage::LoadResources(const ThemeResourceList& names)
{
    THROW_NOT_IMPLEMENTED;
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
