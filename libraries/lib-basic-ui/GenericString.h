/*!********************************************************************

Saucedacity: A Digital Audio Editor

@file GenericString.h
@brief GenericString declarations

Avery King

**********************************************************************/

#ifndef __GENERICUI_GENERICSTRING__
#define __GENERICUI_GENERICSTRING__

#include <wx/string.h>
#include <string>
#include <vector>
#include "GenericObject.h"

namespace GenericUI {

class GenericString : public GenericObject
{
  public:
    GenericString(); /// Default constructor
    GenericString(GenericString& source); /// Copy Constructor
    GenericString(GenericString&& source); /// Move Constructor

    GenericString(wxString&  source); /// Constructs a GenericString from a wxStrimg

    GenericString(std::string&  source); /// Constructs a GenericString from a std::string
    GenericString(std::wstring& source); /// Constructs a GenericString from a std::wstring

    /// Default destructor
    ~GenericString();

    // Member functions
    void DestroyObject() override;

    wxString GetInternalObject() const;

    // Operators
    GenericString& operator=(GenericString& source); /// Copy assignment operator
    GenericString& operator=(GenericString&& source); /// Move assignment operator

    GenericString& operator=(std::string&  source);
    GenericString& operator=(std::wstring& source);
    GenericString& operator=(wxString&  source);

  private:

    /// Internal object flags
    enum
    {
      FlagIsChildObject      = 1 << 0,
      FlagIsAllocatedWithNew = 1 << 1 // if the object was manually allocated with 'new'
    };

    wxString* mString = nullptr;
};

// Typedefs
using GenericStringArray = std::vector<GenericString>;

} // namespace GenericUI

#endif // end __GENERICUI_GENERICSTRING__
