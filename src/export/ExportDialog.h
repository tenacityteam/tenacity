/**********************************************************************

  Tenacity

  ExportDialog.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#ifndef __TENACITY_EXPORTDIALOG_H__
#define __TENACITY_EXPORTDIALOG_H__

#include <wx/event.h>
#include <wx/filedlgcustomize.h>
#include <wx/simplebook.h>

#include "ExportPlugin.h"
#include "../widgets/wxPanelWrapper.h"

class ExportOptionsDialog final : public wxDialogWrapper
{
    private:
        ExportPluginArray& mPlugins;
        wxSimplebook* mBook;
        void Populate();
        int mIndex;

        void OnOK(wxCommandEvent&);
        void OnCancel(wxCommandEvent&);

    public:
        ExportOptionsDialog(ExportPluginArray& plugins);
        void SetIndex(int index);
};

#endif // end __TENACITY_EXPORTDIALOGHOOK_H__
