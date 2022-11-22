/**********************************************************************

  Tenacity: A Digital Audio Editor

  XMLFileWriter.cpp

  Avery King split from XMLWriter.cpp

**********************************************************************/
#include "XMLFileWriter.h"

XMLFileWriter::XMLFileWriter(
   const FilePath &outputPath, const TranslatableString &caption, bool keepBackup )
   : mOutputPath{ outputPath }
   , mCaption{ caption }
   , mKeepBackup{ keepBackup }
// may throw
{
   auto tempPath = wxFileName::CreateTempFileName( outputPath );
   if (!wxFFile::Open(tempPath, wxT("wb")) || !IsOpened())
      ThrowException( outputPath, mCaption );

   if (mKeepBackup) {
      int index = 0;
      wxString backupName;

      do {
         wxFileName outputFn{ mOutputPath };
         index++;
         mBackupName =
         outputFn.GetPath() + wxFILE_SEP_PATH +
         outputFn.GetName() + wxT("_bak") +
         wxString::Format(wxT("%d"), index) + wxT(".") +
         outputFn.GetExt();
      } while( ::wxFileExists( mBackupName ) );

      // Open the backup file to be sure we can write it and reserve it
      // until committing
      if (! mBackupFile.Open( mBackupName, "wb" ) || ! mBackupFile.IsOpened() )
         ThrowException( mBackupName, mCaption );
   }
}


XMLFileWriter::~XMLFileWriter()
{
   // Don't let a destructor throw!
   GuardedCall( [&] {
      if (!mCommitted) {
         auto fileName = GetName();
         if ( IsOpened() )
            CloseWithoutEndingTags();
         ::wxRemoveFile( fileName );
      }
   } );
}

void XMLFileWriter::Commit()
// may throw
{
   PreCommit();
   PostCommit();
}

void XMLFileWriter::PreCommit()
// may throw
{
   while (mTagstack.size()) {
      EndTag(mTagstack[0]);
   }

   CloseWithoutEndingTags();
}

void XMLFileWriter::PostCommit()
// may throw
{
   FilePath tempPath = GetName();
   if (mKeepBackup) {
      if (! mBackupFile.Close() ||
          ! wxRenameFile( mOutputPath, mBackupName ) )
         ThrowException( mBackupName, mCaption );
   }
   else {
      if ( wxFileName::FileExists( mOutputPath ) &&
           ! wxRemoveFile( mOutputPath ) )
         ThrowException( mOutputPath, mCaption );
   }

   // Now we have vacated the file at the output path and are committed.
   // But not completely finished with steps of the commit operation.
   // If this step fails, we haven't lost the successfully written data,
   // but just failed to put it in the right place.
   if (! wxRenameFile( tempPath, mOutputPath ) )
      throw FileException{
         FileException::Cause::Rename, tempPath, mCaption, mOutputPath
      };

   mCommitted = true;
}

void XMLFileWriter::CloseWithoutEndingTags()
// may throw
{
   // Before closing, we first flush it, because if Flush() fails because of a
   // "disk full" condition, we can still at least try to close the file.
   if (!wxFFile::Flush())
   {
      wxFFile::Close();
      ThrowException( GetName(), mCaption );
   }

   // Note that this should never fail if flushing worked.
   if (!wxFFile::Close())
      ThrowException( GetName(), mCaption );
}

void XMLFileWriter::Write(const wxString &data)
// may throw
{
   if (!wxFFile::Write(data, wxConvUTF8) || Error())
   {
      // When writing fails, we try to close the file before throwing the
      // exception, so it can at least be deleted.
      wxFFile::Close();
      ThrowException( GetName(), mCaption );
   }
}
