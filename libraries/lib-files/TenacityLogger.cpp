/**********************************************************************

  Audacity: A Digital Audio Editor

  TenacityLogger.cpp

******************************************************************//**

\class TenacityLogger
\brief TenacityLogger is a thread-safe logger class

Provides thread-safe logging based on the wxWidgets log facility.

*//*******************************************************************/


#include "TenacityLogger.h"

#include "Internat.h"
#include "MemoryX.h"

#include <memory>
#include <mutex>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

//
// TenacityLogger class
//
// By providing an Audacity specific logging class, it can be made thread-safe and,
//     as such, can be used by the ever growing threading within Audacity.
//

TenacityLogger *TenacityLogger::Get()
{
   static std::once_flag flag;
   std::call_once( flag, []{
      // wxWidgets will clean up the logger for the main thread, so we can say
      // safenew.  See:
      // http://docs.wxwidgets.org/3.0/classwx_log.html#a2525bf54fa3f31dc50e6e3cd8651e71d
      std::unique_ptr < wxLog > // DELETE any previous logger
         { wxLog::SetActiveTarget(safenew TenacityLogger) };
   } );

   // Use dynamic_cast so that we get a NULL ptr in case our logger
   // is no longer the target.
   return dynamic_cast<TenacityLogger *>(wxLog::GetActiveTarget());
}

TenacityLogger::TenacityLogger()
:  wxEvtHandler(),
   wxLog()
{
   mUpdated = false;
}

TenacityLogger::~TenacityLogger()  = default;

void TenacityLogger::Flush()
{
   if (mUpdated && mListener && mListener())
      mUpdated = false;
}

auto TenacityLogger::SetListener(Listener listener) -> Listener
{
   auto result = std::move(mListener);
   mListener = std::move(listener);
   return result;
}

void TenacityLogger::DoLogText(const wxString & str)
{
   if (!wxIsMainThread()) {
      wxMutexGuiEnter();
   }

   if (mBuffer.empty()) {
      wxString stamp;

      TimeStamp(&stamp);

      mBuffer << stamp << "Tenacity " << AUDACITY_VERSION_STRING << wxT("\n");
   }

   mBuffer << str << wxT("\n");

   mUpdated = true;

   Flush();

   if (!wxIsMainThread()) {
      wxMutexGuiLeave();
   }
}

bool TenacityLogger::SaveLog(const wxString &fileName) const
{
   wxFFile file(fileName, wxT("w"));

   if (file.IsOpened()) {
      file.Write(mBuffer);
      file.Close();
      return true;
   }

   return false;
}

bool TenacityLogger::ClearLog()
{
   mBuffer = wxEmptyString;
   DoLogText(wxT("Log Cleared."));

   return true;
}

wxString TenacityLogger::GetLog(int count)
{
   if (count == 0)
   {
      return mBuffer;
   }

   wxString buffer;

   auto lines = wxStringTokenize(mBuffer, wxT("\r\n"), wxTOKEN_RET_DELIMS);
   for (int index = lines.size() - 1; index >= 0 && count > 0; --index, --count)
   {
      buffer.Prepend(lines[index]);
   }

   return buffer;
}
