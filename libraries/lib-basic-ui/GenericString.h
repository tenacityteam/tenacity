/*!********************************************************************

Tenacity: A Digital Audio Editor

@file GenericString.h
@brief GenericString declarations

Avery King

**********************************************************************/

#ifndef __GENERICUI_GENERICSTRING__
#define __GENERICUI_GENERICSTRING__

#include <wx/string.h>
#include <string>
#include <vector>

namespace GenericUI {

class GenericString
{
  public:
    GenericString(); /// Default constructor
    GenericString(GenericString& source); /// Copy Constructor
    GenericString(GenericString&& source); /// Move Constructor

    GenericString(wxString& source); /// Constructs a GenericString from a wxStrimg

    GenericString(const char*   source); /// Constructs a GenericString from a string literal
    GenericString(std::string&  source); /// Constructs a GenericString from a std::string
    GenericString(std::wstring& source); /// Constructs a GenericString from a std::wstring

    // Operators
    GenericString& operator=(GenericString& source); /// Copy assignment operator
    GenericString& operator=(GenericString&& source); /// Move assignment operator

    GenericString& operator=(std::string&  source);
    GenericString& operator=(std::wstring& source);
    GenericString& operator=(wxString&  source);

    /// Same as GetChar(pos)
    char operator[] (size_t pos);

    // Member functions
    
    /// Returns the internal string used by GenericString
    wxString& GetInternalString();

    /// Returns a reference to the character as `pos`.
    char GetChar(size_t pos);

  private:
    wxString mString;
};

// Typedefs
using GenericStringArray = std::vector<GenericString>;

} // namespace GenericUI

#endif // end __GENERICUI_GENERICSTRING__
