/**********************************************************************

  Tenacity: A Digital Audio Editor

  AuStaticText.h

  Avery King split from Theme.h

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#pragma once

#include <wx/window.h>
#include <wx/dcclient.h>

/// Like wxStaticText, except it allows for theming unlike wxStaticText
class TENACITY_DLL_API AuStaticText : public wxWindow
{
    public:
        AuStaticText(wxWindow* parent, wxString text);
        bool AcceptsFocus() const override { return false; }
};
