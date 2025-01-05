/*  SPDX-License-Identifier: GPL-2.0-or-later */
/*!********************************************************************

  Tenacity

  WelcomeDialog.h

  Vitaly Sverchinsky
  Avery King

**********************************************************************/

#pragma once

#include "wxPanelWrapper.h"

class wxCheckBox;
class ShuttleGui;
class AudacityProject;

class WelcomeDialog final : public wxDialogWrapper
{
   wxCheckBox* mDontShowAgain{};
public:
   WelcomeDialog(wxWindow* parent, wxWindowID id);
   ~WelcomeDialog() override;

   static void Show(AudacityProject& project);

private:
   void Populate(ShuttleGui& S);
   void OnOK(wxCommandEvent&);

   DECLARE_EVENT_TABLE()
};
