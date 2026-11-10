/**********************************************************************

  Tenacity

  ThemePackage.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ThemePackage.h"

#include <cassert>

#include <simdjson.h>

#include "exceptions/ArchiveError.h"
#include "exceptions/IncompatibleTheme.h"
#include "exceptions/InvalidState.h"

using namespace ThemeExceptions;
using namespace simdjson;

#define THROW_NOT_IMPLEMENTED throw std::runtime_error("Not implemented")

ThemePackage::ThemePackage()
: mPackageArchive{nullptr},
  mColors{},
  mSubthemes{},
  mRecognizedColors{},
  mRecognizedBitmaps{},
  mIsMultiThemePackage{false}
{
}

ThemePackage& ThemePackage::operator=(ThemePackage&& other)
{
    mPackageArchive = other.mPackageArchive;
    other.mPackageArchive = nullptr;

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

    mColors = std::move(other.mColors);
}

std::vector<char> ThemePackage::ReadFileFromArchive(const std::string& name)
{
    std::vector<char> fileData;
    zip_file_t* file;
    zip_stat_t fileInfo;
    zip_int64_t bytesRead;
    int error;

    // Get the file's size
    error = zip_stat(mPackageArchive, name.c_str(), ZIP_STAT_SIZE, &fileInfo);
    if (error != 0)
    {
        return fileData;
    }

    // Allocate our data buffer to the file's size
    fileData.resize(fileInfo.size);

    // Open the file
    file = zip_fopen(mPackageArchive, name.c_str(), 0);
    if (!file) 
    {
        zip_fclose(file);
        fileData.clear();
        return fileData;
    }

    // Read the file
    bytesRead = zip_fread(file, fileData.data(), fileInfo.size);
    if (bytesRead != fileInfo.size)
    {
        zip_fclose(file);
        fileData.clear();
        return fileData;
    }

    zip_fclose(file);
    return fileData;
}

dom::element ThemePackage::GetParsedJsonData(const std::string& jsonFile)
{
    std::vector<char> data = ReadFileFromArchive(jsonFile);
    if (data.empty())
    {
        throw ArchiveError(ArchiveError::Type::OperationalError);
    }

    simdjson::padded_string paddedData(data.data(), data.size());
    return mParser.parse(paddedData);
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
    dom::element info = GetParsedJsonData("info.json");

    // Check the minimum required app version
    try
    {
        dom::array versionArray = info["minAppVersion"];
        uint64_t requiredMajorVersion = versionArray.at(0);
        uint64_t requiredReleaseVersion = versionArray.at(1);

        // Check the major version number
        //
        // Note: there shouldn't be any changes significant enough across
        // different revisions (e.g., 1.4.1 vs 1.4.2, etc).
        if (requiredMajorVersion > TENACITY_VERSION || requiredReleaseVersion > TENACITY_RELEASE)
        {
            // Will propogate outside this member function as we only catch
            // simdjson_error exceptions.
            throw IncompatibleTheme(
                requiredMajorVersion,
                requiredReleaseVersion,
                requiredReleaseVersion
            );
        }
    } catch (const simdjson_error& err)
    {
        ClosePackage();
        throw ArchiveError(ArchiveError::Type::Invalid);
    }

    // Check if the theme package contains a "subthemes" element. If it does, it
    // contains multiple subthemes.
    try
    {
        dom::array subthemes = info["subthemes"];
        for (std::string_view subtheme : subthemes)
        {
            mSubthemes.push_back(subtheme);
        }

        mIsMultiThemePackage = true;
    } catch (const simdjson_error&)
    {
        mIsMultiThemePackage = false;
    }

    // Get the theme name
    if (!mIsMultiThemePackage)
    {
        try
        {
            mName = info["name"];
        } catch (const simdjson_error&)
        {
            ClosePackage();
            throw ArchiveError(ArchiveError::Type::Invalid);
        }
    }

    // Finally, preload all color values.
    //
    // This is more oriented towards multi-theme packages, in which this can
    // help us load themes faster, but it will certainly benefit single-theme
    // packages as well.
    try
    {
        dom::object colors = GetParsedJsonData("colors.json");
        if (mRecognizedColors.size() > 0)
        {
            std::string_view currentFoundColor;
            for (const auto& recognizedColor : mRecognizedColors)
            {
                try
                {
                    mColors[recognizedColor] = colors[recognizedColor];
                } catch (const simdjson_error&)
                {
                    // Ignore it
                    continue;
                }
            }
        } else
        {
            for (auto color = colors.begin(); color < colors.end(); color++)
            {
                mColors[color.key()] = color.value();
            }
        }
    } catch(const simdjson_error&)
    {
        // There should be a colors.json at the root of the archive in a
        // single-theme package. Throw an exception if that's the case.
        if (!mIsMultiThemePackage)
        {
            ClosePackage();
            throw ArchiveError(ArchiveError::Type::Invalid);
        }
    }
}

void ThemePackage::ClosePackage()
{
    if (mPackageArchive)
    {
        zip_close(mPackageArchive);
    }

    mPackageArchive = nullptr;

    mColors.clear();
    mSubthemes.clear();
    mRecognizedColors.clear();
    mRecognizedBitmaps.clear();
}

void ThemePackage::SetRecognizedResources(
    std::vector<std::string>& recognizedColors,
    std::vector<std::string>& recognizedBitmaps
)
{
    mRecognizedColors = recognizedColors;
    mRecognizedBitmaps = recognizedBitmaps;
}

Theme ThemePackage::LoadTheme(const std::string& theme)
{
    // Precondition: the package must be open first
    if (!IsOpen()) throw InvalidState();

    // TODO: Finish the LoadTheme() rewrite using simdjson
    return {};
}

bool ThemePackage::IsMultiThemePackage() const
{
    if (!IsOpen()) throw InvalidState();

    return mIsMultiThemePackage;
}
