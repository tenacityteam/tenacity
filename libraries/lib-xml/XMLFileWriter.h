/**********************************************************************

  Saucedacity: A Digital Audio Editor

  XMLFileWriter.h

  Avery King split from XMLWriter.h

**********************************************************************/

#pragma once

#include "XMLWriter.h"

/** @brief Wrapper to output XML data to files.

 * This writes to a provisional file, and replaces the previously existing
 * contents by a file rename in Commit() only after all writes succeed.
 * The original contents may also be retained at a backup path name, as
 * directed by the optional constructor argument.
 * If it is destroyed before Commit(), then the provisional file is removed.
 * If the construction and all operations are inside a GuardedCall or event
 * handler, then the default delayed handler action in case of exceptions will
 * notify the user of problems.
**/
class XML_API XMLFileWriter final : private wxFFile, public XMLWriter
{
    public:
        /// The caption is for message boxes to show in case of errors.
        /// Might throw.
        XMLFileWriter(
            const FilePath &outputPath, const TranslatableString &caption,
            bool keepBackup = false );

        virtual ~XMLFileWriter();

        /// Close all tags and then close the file.
        /// Might throw.  If not, then create
        /// or modify the file at the output path.
        /// Composed of two steps, PreCommit() and PostCommit()
        void Commit();

        /// Does the part of Commit that might fail because of exhaustion of space
        void PreCommit();

        /// Does other parts of Commit that are not likely to fail for exhaustion
        /// of space, but might for other reasons
        void PostCommit();

        /// Write to file. Might throw.
        void Write(const wxString &data) override;

        FilePath GetBackupName() const { return mBackupName; }

    private:
        void ThrowException(
            const wxFileName &fileName, const TranslatableString &caption)
        {
            throw FileException{ FileException::Cause::Write, fileName, caption };
        }

        /// Close file without automatically ending tags.
        /// Might throw.
        void CloseWithoutEndingTags(); // for auto-save files

        const FilePath mOutputPath;
        const TranslatableString mCaption;
        FilePath mBackupName;
        const bool mKeepBackup;

        wxFFile mBackupFile;

        bool mCommitted{ false };
};
