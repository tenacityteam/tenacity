/*!********************************************************************

Tenacity: A Digital Audio Editor

@file GenericString.cpp
@brief GenericString implementation.

Avery King

**********************************************************************/
#include "GenericString.h"

namespace GenericUI {

GenericString::GenericString()
{
}

GenericString::GenericString(GenericString& source)
{
  mString = source.mString;
}

GenericString::GenericString(GenericString&& source)
{
  mString = source.mString;
  source.mString = wxEmptyString;
}

GenericString::GenericString(wxString& source)
{
  mString = source;
}

GenericString::GenericString(const char* source)
{
  std::string dummy = source;
  mString = wxString(dummy);
}

GenericString::GenericString(std::string& source)
{
  mString = wxString(source);
}

GenericString::GenericString(std::wstring& source)
{
  mString = wxString(source);
}

//// Member functions /////////////////////////////////////////////////////////
wxString& GenericString::GetInternalString()
{
  return mString;
}

char GenericString::GetChar(size_t pos)
{
  return mString.GetChar(pos).GetValue();
}

//// Operators Overloads //////////////////////////////////////////////////////
GenericString& GenericString::operator=(GenericString& source)
{
  mString = source.mString;
  return *this;
}

GenericString& GenericString::operator=(GenericString&& source)
{
  mString = source.mString;
  source.mString = wxEmptyString;

  return *this;
}

GenericString& GenericString::operator=(wxString& source)
{
  mString = source;
  return *this;
}

GenericString& GenericString::operator=(std::string& source)
{

  mString = wxString(source);
  return *this;
}

GenericString& GenericString::operator=(std::wstring& source)
{
  mString = wxString(source);
  return *this;
}

char GenericString::operator[] (size_t pos)
{
  return GetChar(pos);
}

} // namespace GenericUI
