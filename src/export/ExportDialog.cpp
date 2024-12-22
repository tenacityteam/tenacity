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
#include "../widgets/HelpSystem.h"

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
    // Events
    Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnOK, this, wxID_OK);
    Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnCancel, this, wxID_CANCEL);
    Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnHelp, this, wxID_HELP);

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

        S.AddStandardButtons(eOkButton | eCancelButton | eHelpButton);
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

void ExportOptionsDialog::OnHelp(wxCommandEvent&)
{
    HelpSystem::ShowHelp(this, L"Importing_and_Exporting", true);
}

void ExportOptionsDialog::SetIndex(int index)
{
    mIndex = index;
    mBook->ChangeSelection(mIndex);

    // Most likey we'll be showing the dialog after this member is called.
    Fit();
}

///////////////////////////////////////////////////////////////////////////////
