/**********************************************************************

  Saucedacity: A Digital Audio Editor

  AuStaticText.h

  Avery King split from Theme.h

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#pragma once

#include <wx/window.h>
#include <wx/dcclient.h>

class SAUCEDACITY_DLL_API AuStaticText : public wxWindow
{
    public:
        AuStaticText(wxWindow* parent, wxString text);

        void OnPaint(wxPaintEvent & evt);
        bool AcceptsFocus() const override { return false; }

        void OnErase(wxEraseEvent& event)
        {
            static_cast<void>(event);
        };

        DECLARE_EVENT_TABLE();
};
