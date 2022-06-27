/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  FFmpegNotFoundDialog.h

  Avery King split from FFmpeg.h

  This contains the definition for FFmpegNotFoundDialog, which is the dialog
  that displays to the user that FFmpeg was not found.

*******************************************************************************/

#ifndef __SAUCEDACITY_FFMPEGNOTFOUNDDIALOG_H__
#define __SAUCEDACITY_FFMPEGNOTFOUNDDIALOG_H__

#ifdef USE_FFMPEG

#include "../shuttle/ShuttleGui.h"
#include "../widgets/wxPanelWrapper.h"
#include <wx/checkbox.h>

/// If Saucedacity failed to load libav*, this dialog shows up and tells user
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

#endif // end USE_FFMPEG

#endif // end __SAUCEDACITY_FFMPEGNOTFOUNDDIALOG_H__
