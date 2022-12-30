/**********************************************************************

  Tenacity

  ExportDialog.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "ExportDialog.h"

#include <wx/bmpbuttn.h>

// Tenacity libraries
#include <lib-strings/Internat.h>

#include "../shuttle/ShuttleGui.h"
#include "../theme/AllThemeResources.h"
#include "../theme/Theme.h"

//// ExportOptionsDialog //////////////////////////////////////////////////////

ExportOptionsDialog::ExportOptionsDialog(ExportPluginArray& plugins)
: wxDialogWrapper(
                  nullptr,
                  wxID_ANY,
                  XO("Export Options")
),
mPlugins(plugins),
mBook(nullptr)
{
    Populate();
}

void ExportOptionsDialog::Populate()
{
    ShuttleGui S(this, eIsCreating);

    S.StartStatic(XO("Format Options"), 1);
    {
        S.StartHorizontalLay(wxEXPAND);
        {
            mBook = S.Position(wxEXPAND).StartSimplebook();
            {
                for (const auto &pPlugin : mPlugins)
                {
                    for (int j = 0; j < pPlugin->GetFormatCount(); j++)
                    {
                        // Name of simple book page is not displayed
                        S.StartNotebookPage( {} );
                        {
                            pPlugin->OptionsCreate(S, j);
                        }
                        S.EndNotebookPage();
                    }
                }
            }
            S.EndSimplebook();
        }
        S.EndHorizontalLay();

        S.StartHorizontalLay(wxALIGN_RIGHT);
        {
            auto okButton     = S.AddButton(XO("OK"));
            auto cancelButton = S.AddButton(XO("Cancel"));
            auto helpButton   = new wxBitmapButton(S.GetParent(), wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));

            okButton->Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnOK, this);
            cancelButton->Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnCancel, this);

            helpButton->SetToolTip( XO("Help").Translation() );
            helpButton->SetLabel(XO("Help").Translation()); // for screen readers
            // S.Position(wxALIGN_TOP).AddWindow(helpButton);
            S.AddWindow(helpButton);
        }
        S.EndHorizontalLay();
    }
    S.EndStatic();
}

void ExportOptionsDialog::OnOK(wxCommandEvent&)
{
    EndModal(wxID_OK);
}

void ExportOptionsDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}

void ExportOptionsDialog::SetIndex(int index)
{
    mIndex = index;
    mBook->ChangeSelection(mIndex);

    // Most likey we'll be showing the dialog after this member is called.
    Fit();
}

///////////////////////////////////////////////////////////////////////////////
