/**********************************************************************

  Tenacity

  LabelTrackControlPanel.cpp

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#include "LabelTrackControlPanel.h"
#include "ShuttleGui.h"
#include "newtracks/TrackControlPanel.h"

#include <wx/fontenum.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>

enum : int { DefaultFontSize = 0 }; // System preferred

// Event IDs
enum {
    OnSetFontID
};

wxFont GetFont(const wxString &faceName, int size = DefaultFontSize)
{
    wxFontEncoding encoding;
    if (faceName.empty())
    {
        encoding = wxFONTENCODING_DEFAULT;
    } else
    {
        encoding = wxFONTENCODING_SYSTEM;
    }

    auto fontInfo = size == 0 ? wxFontInfo() : wxFontInfo(size);
    fontInfo
        .Encoding(encoding)
        .FaceName(faceName);

    return wxFont(fontInfo);
}

LabelTrackControlPanel::LabelTrackControlPanel(
    wxWindow* parent, AudacityProject& project, std::shared_ptr<Track>& track
) : TrackControlPanel(parent, project, track)
{
    AddOptionsSeparator();
    AddOption(XXO("&Font..."), &LabelTrackControlPanel::OnFontDialog, this, OnSetFontID);
}

void LabelTrackControlPanel::OnFontDialog(wxCommandEvent&)
{
    // Small helper class to enumerate all fonts in the system
    // We use this because the default implementation of
    // wxFontEnumerator::GetFacenames() has changed between wx2.6 and 2.8
    // TODO: Check if this class is still needed
    class FontEnumerator : public wxFontEnumerator
    {
        public:
            explicit FontEnumerator(wxArrayString* fontNames) :
                mFontNames(fontNames) {}

            bool OnFacename(const wxString& font) override
            {
                mFontNames->push_back(font);
                return true;
            }

        private:
            wxArrayString* mFontNames;
    };

    wxArrayString facenames;
    FontEnumerator fontEnumerator(&facenames);
    fontEnumerator.EnumerateFacenames(wxFONTENCODING_SYSTEM, false);

    wxString facename = gPrefs->Read(wxT("/GUI/LabelFontFacename"), wxT(""));

    // Correct for empty facename, or bad preference file:
    // get the name of a really existing font, to highlight by default
    // in the list box
    facename = ::GetFont(facename).GetFaceName();

    long fontsize = gPrefs->Read(
        wxT("/GUI/LabelFontSize"), static_cast<int>(DefaultFontSize)
    );

    /* i18n-hint: (noun) This is the font for the label track.*/
    wxDialogWrapper dlg(this, wxID_ANY, XO("Label Track Font"));
    dlg.SetName();
    ShuttleGui S(&dlg, eIsCreating);
    wxListBox *lb;
    wxSpinCtrl *sc;

    S.StartVerticalLay(true);
    {
        S.StartMultiColumn(2, wxEXPAND);
        {
            S.SetStretchyRow(0);
            S.SetStretchyCol(1);

            /* i18n-hint: (noun) The name of the typeface*/
            S.AddPrompt(XXO("Face name"));
            lb = safenew wxListBox(S.GetParent(), wxID_ANY,
                wxDefaultPosition,
                wxDefaultSize,
                facenames,
                wxLB_SINGLE);

            lb->SetSelection( make_iterator_range( facenames ).index( facename ));
            S
                .Name(XO("Face name"))
                .Position(  wxALIGN_LEFT | wxEXPAND | wxALL )
                .AddWindow(lb);

            /* i18n-hint: (noun) The size of the typeface*/
            S.AddPrompt(XXO("Face size"));
            sc = safenew wxSpinCtrl(S.GetParent(), wxID_ANY,
                wxString::Format(wxT("%ld"), fontsize),
                wxDefaultPosition,
                wxDefaultSize,
                wxSP_ARROW_KEYS,
                8, 48, fontsize);
            S
                .Name(XO("Face size"))
                .Position( wxALIGN_LEFT | wxALL )
                .AddWindow(sc);
        }
        S.EndMultiColumn();
        S.AddStandardButtons();
    }
    S.EndVerticalLay();

    dlg.Layout();
    dlg.Fit();
    dlg.CenterOnParent();

    if (dlg.ShowModal() == wxID_CANCEL)
    {
        return;
    }

    gPrefs->Write(wxT("/GUI/LabelFontFacename"), lb->GetStringSelection());
    gPrefs->Write(wxT("/GUI/LabelFontSize"), sc->GetValue());
    gPrefs->Flush();

   // TODO: Reset font
   // This will likely need to be handled by the content panel when that gets
   // implemented.
   // LabelTrackView::ResetFont();
}
