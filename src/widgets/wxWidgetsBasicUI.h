/*!********************************************************************

Audacity: A Digital Audio Editor

@file wxWidgetsBasicUI.h
@brief Implementation of BasicUI using wxWidgets

Paul Licameli

**********************************************************************/
#ifndef __WXWIDGETS_BASIC_UI__
#define __WXWIDGETS_BASIC_UI__

// Saucedacity libraries
#include <lib-basic-ui/BasicUI.h>

class wxWindow;

//! Window placement information for wxWidgetsBasicUI can be constructed from a wxWindow pointer
struct SAUCEDACITY_DLL_API wxWidgetsWindowPlacement final
: GenericUI::WindowPlacement {
   wxWidgetsWindowPlacement() = default;
   explicit wxWidgetsWindowPlacement( wxWindow *pWindow )
      : pWindow{ pWindow }
   {}

   ~wxWidgetsWindowPlacement() override;
   wxWindow *pWindow{};
};

//! An implementation of GenericUI::Services in terms of the wxWidgets toolkit
/*! This is a singleton that doesn't need SAUCEDACITY_DLL_API visibility */
class wxWidgetsBasicUI final : public GenericUI::Services {
public:
   ~wxWidgetsBasicUI() override;

protected:
   void DoCallAfter(const GenericUI::Action &action) override;
   void DoYield() override;
   void DoShowErrorDialog(const GenericUI::WindowPlacement &placement,
      const TranslatableString &dlogTitle,
      const TranslatableString &message,
      const ManualPageID &helpPage,
      const GenericUI::ErrorDialogOptions &options) override;
   GenericUI::MessageBoxResult DoMessageBox(
      const TranslatableString &message,
      GenericUI::MessageBoxOptions options) override;
   std::unique_ptr<GenericUI::ProgressDialog>
   DoMakeProgress(const TranslatableString & title,
      const TranslatableString &message,
      unsigned flags,
      const TranslatableString &remainingLabelText) override;
   std::unique_ptr<GenericUI::GenericProgressDialog>
   DoMakeGenericProgress(const GenericUI::WindowPlacement &placement,
      const TranslatableString &title,
      const TranslatableString &message) override;
};

#endif
