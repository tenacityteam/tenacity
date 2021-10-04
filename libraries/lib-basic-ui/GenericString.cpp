/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericString.cpp
@brief GenericString implementation.

Avery King

**********************************************************************/
#include "GenericString.h"

namespace GenericUI {

GenericString::GenericString()
{
  mString = new wxString;
  mFlags |= FlagIsAllocatedWithNew;
}

GenericString::GenericString(GenericString& source)
{
  mString = source.mString;
  mFlags = source.mFlags;
}

GenericString::GenericString(GenericString&& source)
{
  mString = source.mString;
  mFlags = source.mFlags;

  source.DestroyObject();
}

GenericString::GenericString(wxString& source)
{
  mString = new wxString(source);
}

GenericString::GenericString(std::string& source)
{
  mString = new wxString(source);
  mFlags |= FlagIsAllocatedWithNew;
}

GenericString::GenericString(std::wstring& source)
{
  mString = new wxString(source);
  mFlags |= FlagIsAllocatedWithNew;
}

GenericString::~GenericString()
{
  DestroyChildObjects();
  DestroyObject();
}

//// Member functions /////////////////////////////////////////////////////////
void GenericString::DestroyObject()
{
  if ((mFlags & FlagIsAllocatedWithNew) == FlagIsAllocatedWithNew)
  {
    delete mString;
  }

  mString = nullptr;
  mFlags = 0;
  ClearChildren();
}

//// Operators Overloads //////////////////////////////////////////////////////
GenericString& GenericString::operator=(GenericString& source)
{
  if ((mFlags & FlagIsAllocatedWithNew) == FlagIsAllocatedWithNew)
  {
    delete mString;
  }

  mString = source.mString;
  mFlags = source.mFlags;

  return *this;
}

GenericString& GenericString::operator=(GenericString&& source)
{
  if ((mFlags & FlagIsAllocatedWithNew) == FlagIsAllocatedWithNew)
  {
    delete mString;
  }

  mString = source.mString;
  mFlags = source.mFlags;

  source.DestroyObject();

  return *this;
}

GenericString& GenericString::operator=(wxString& source)
{
  if (!(mFlags & FlagIsAllocatedWithNew))
  {
    mString = &source;
  }


  return *this;
}

GenericString& GenericString::operator=(std::string& source)
{
  if ((mFlags & FlagIsAllocatedWithNew) == FlagIsAllocatedWithNew)
  {
    delete mString;
  }

  mString = new wxString(source);

  return *this;
}

GenericString& GenericString::operator=(std::wstring& source)
{
  if ((mFlags & FlagIsAllocatedWithNew) == FlagIsAllocatedWithNew)
  {
    delete mString;
  }

  mString = new wxString(source);

  return *this;
}

} // namespace GenericUI
