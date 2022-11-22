//
// Copied from wxWidgets 3.1.5 and modified for Tenacity
//
/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/filedlg.h
// Purpose:     wxFileDialog class
// Author:      Julian Smart
// Modified by: Leland Lucius, Avery King
// Created:     01/02/97
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _SAUCEDACITY_FILEDIALOGPRIVATE_H
#define _SAUCEDACITY_FILEDIALOGPRIVATE_H

#include "../FileDialog.h"

//-------------------------------------------------------------------------
// FileDialog
//-------------------------------------------------------------------------

class TENACITY_DLL_API FileDialog : public FileDialogBase
{
public:
    FileDialog();
    FileDialog(wxWindow *parent,
                 const wxString& message = wxFileSelectorPromptStr,
                 const wxString& defaultDir = wxEmptyString,
                 const wxString& defaultFile = wxEmptyString,
                 const wxString& wildCard = wxFileSelectorDefaultWildcardStr,
                 long style = wxFD_DEFAULT_STYLE,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& sz = wxDefaultSize,
                 const wxString& name = wxFileDialogNameStr);

    virtual void GetPaths(wxArrayString& paths) const wxOVERRIDE;
    virtual void GetFilenames(wxArrayString& files) const wxOVERRIDE;
    virtual bool SupportsExtraControl() const wxOVERRIDE { return true; }
    void MSWOnInitDialogHook(WXHWND hwnd);

    virtual int ShowModal() wxOVERRIDE;

    // wxMSW-specific implementation from now on
    // -----------------------------------------

    // called from the hook procedure on CDN_INITDONE reception
    virtual void MSWOnInitDone(WXHWND hDlg);

    // called from the hook procedure on CDN_SELCHANGE.
    void MSWOnSelChange(WXHWND hDlg);

    // called from the hook procedure on CDN_TYPECHANGE.
    void MSWOnTypeChange(WXHWND hDlg, int nFilterIndex);

protected:

    virtual void DoMoveWindow(int x, int y, int width, int height) wxOVERRIDE;
    virtual void DoCentre(int dir) wxOVERRIDE;
    virtual void DoGetSize( int *width, int *height ) const wxOVERRIDE;
    virtual void DoGetPosition( int *x, int *y ) const wxOVERRIDE;

private:
    void Init();

    wxArrayString m_fileNames;

    // remember if our SetPosition() or Centre() (which requires special
    // treatment) was called
    bool m_bMovedWindow;
    int m_centreDir;        // nothing to do if 0

    wxDECLARE_DYNAMIC_CLASS(FileDialog);
    wxDECLARE_NO_COPY_CLASS(FileDialog);
};

#endif // _SAUCEDACITY_FILEDIALOGPRIVATE_H
