/**********************************************************************

  Audacity: A Digital Audio Editor

  SelectionBar.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_SELECTION_BAR__
#define __AUDACITY_SELECTION_BAR__

#include <wx/defs.h>

#include "ToolBar.h"

// Tenacity libraries
#include <lib-utility/Observer.h>

// Column for 
//   Selection fields
//   Vertical Line

class wxChoice;
class wxCommandEvent;
class wxDC;
class wxSizeEvent;
class wxStaticText;

class AuStaticText;
class TenacityProject;
class SelectionBarListener;
class NumericTextCtrl;

class TENACITY_DLL_API SelectionBar final : public ToolBar {

 public:
   SelectionBar( TenacityProject &project );
   virtual ~SelectionBar();

   static SelectionBar &Get( TenacityProject &project );
   static const SelectionBar &Get( const TenacityProject &project );

   void Create(wxWindow *parent) override;

   void Populate() override;
   void Repaint(wxDC * /* dc */) override {};
   void EnableDisableButtons() override {};
   void UpdatePrefs() override;

   void SetTimes(double start, double end);
   void SetSelectionFormat(const NumericFormatSymbol & format);
   void SetListener(SelectionBarListener *l);
   void RegenerateTooltips() override {}

 private:
   AuStaticText * AddTitle( const TranslatableString & Title,
      wxSizer * pSizer );
   NumericTextCtrl * AddTime( const TranslatableString &Name, int id, wxSizer * pSizer );

   void SetSelectionMode(int mode);
   void ShowHideControls(int mode);
   void SetDrivers( int driver1, int driver2 );
   void ValuesToControls();
   void OnUpdate(wxCommandEvent &evt);
   void OnChangedTime(wxCommandEvent &evt);

   void OnRate(double newRate);
   void OnChoice(wxCommandEvent & event);
   void OnFocus(wxFocusEvent &event);
   void OnCaptureKey(wxCommandEvent &event);
   void OnSize(wxSizeEvent &evt);
   void OnIdle( wxIdleEvent &evt );

   void ModifySelection(int newDriver, bool done = false);
   void SelectionModeUpdated();

   SelectionBarListener * mListener;
   Observer::Subscription mProjectRateSubscription;
   double mRate;
   double mStart, mEnd, mLength, mCenter;

   // These two numbers say which two controls 
   // drive the other two.
   int mDrive1;
   int mDrive2;

   int mSelectionMode{ 0 };
   int mLastSelectionMode{ 0 };

   NumericTextCtrl   *mStartTime;
   NumericTextCtrl   *mCenterTime;
   NumericTextCtrl   *mLengthTime;
   NumericTextCtrl   *mEndTime;
   wxChoice          *mChoice;

   wxString mLastValidText;

 public:

   DECLARE_CLASS(SelectionBar)
   DECLARE_EVENT_TABLE()
};

#endif

