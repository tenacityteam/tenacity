/**********************************************************************

  Tenacity

  SettingsBar.h

  Avery King extracted from SelectionBar.h

**********************************************************************/

#pragma once

#include <wx/defs.h>

#include "ToolBar.h"

// Column for 
//   Project rate
//   Snap To

class wxChoice;
class wxComboBox;
class wxCommandEvent;
class wxDC;
class wxSizeEvent;
class wxStaticText;

class AuStaticText;
class TenacityProject;
class SettingsBarListener;

class TENACITY_DLL_API SettingsBar final : public ToolBar {

 public:
   SettingsBar( TenacityProject &project );
   ~SettingsBar() override;

   static SettingsBar &Get( TenacityProject &project );
   static const SettingsBar &Get( const TenacityProject &project );

   void Create(wxWindow *parent) override;

   void Populate() override;
   void Repaint(wxDC * /* dc */) override {};
   void EnableDisableButtons() override {};
   void UpdatePrefs() override;

   void SetSnapTo(int);
   void SetRate(double rate);
   void SetListener(SettingsBarListener *l);
   void RegenerateTooltips() override {};

 private:
   AuStaticText * AddTitle( const TranslatableString & Title,
      wxSizer * pSizer );
   void AddVLine(  wxSizer * pSizer );

   void OnUpdate(wxCommandEvent &evt);

   void OnRate(wxCommandEvent & event);
   void OnSnapTo(wxCommandEvent & event);
   void OnFocus(wxFocusEvent &event);
   void OnCaptureKey(wxCommandEvent &event);
   void OnSize(wxSizeEvent &evt);

   void UpdateRates();

   SettingsBarListener * mListener;
   double mRate;

   wxStaticText      *mProxy;
   wxComboBox        *mRateBox;
   wxChoice          *mSnapTo;
   wxWindow          *mRateText;

   wxString mLastValidText;

 public:

   DECLARE_CLASS(SettingsBar)
   DECLARE_EVENT_TABLE()
};
