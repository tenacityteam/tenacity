/***********************************************************************

  Saucedacity: A Digital Audio Editor

  SourceOutputStream.h

  Avery King split from Theme.cpp

  This file is licensed under the wxWidgets license, see License.txt

*//*****************************************************************//**

\class SourceOutputStream
\brief Allows us to capture output of the Save .png and 'pipe' it into
our own output function which gives a series of numbers.

*//********************************************************************/

#pragma once

#include <wx/stream.h>
#include <lib-files/FileNames.h>

/** \brief Helper class based on wxOutputStream used to get a png file in text format.
 *
 * This class is currently used by Theme to pack its images into the image cache.
 *  Perhaps someday we will improve FlowPacker and make it more flexible, and use it
 * for toolbar and window layouts too.
 *
 * The trick used here is that wxWidgets can write a PNG image to a stream.
 * By writing to a custom stream, we get to see each byte of data in turn, convert
 * it to text, put in commas, and then write that out to our own text stream.
 * 
 **/
class SourceOutputStream final : public wxOutputStream
{
    public:
        SourceOutputStream(){};

        int OpenFile(const FilePath & Filename);
        virtual ~SourceOutputStream();

    protected:
        size_t OnSysWrite(const void *buffer, size_t bufsize) override;

        wxFile File;
        int nBytes;
};
