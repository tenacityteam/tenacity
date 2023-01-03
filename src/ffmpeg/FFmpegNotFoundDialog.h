/*******************************************************************************

  Tenacity: A Digital Audio Editor

  FFmpegNotFoundDialog.h

  Avery King split from FFmpeg.h

  This contains the definition for FFmpegNotFoundDialog, which is the dialog
  that displays to the user that FFmpeg was not found.

*******************************************************************************/

#ifndef __TENACITY_FFMPEGNOTFOUNDDIALOG_H__
#define __TENACITY_FFMPEGNOTFOUNDDIALOG_H__

#include "../shuttle/ShuttleGui.h"
#include "../widgets/wxPanelWrapper.h"
#include <wx/checkbox.h>

/// If Tenacity failed to load libav*, this dialog shows up and tells user
/// about that. It will pop-up again and again until it is disabled.
class FFmpegNotFoundDialog final : public wxDialogWrapper
{
public:

   FFmpegNotFoundDialog(wxWindow *parent);

   void PopulateOrExchange(ShuttleGui & S);
   void OnOk(wxCommandEvent & WXUNUSED(event));

private:
   wxCheckBox *mDontShow;

   DECLARE_EVENT_TABLE()
};

#endif // end __SAUCEDACITY_FFMPEGNOTFOUNDDIALOG_H__
