/**********************************************************************

  Tenacity: A Digital Audio Editor

  FFmpegNotFoundDialog.cpp

  Avery King split from FFmpeg.cpp

**********************************************************************/

#include "FFmpegNotFoundDialog.h"
#include "FFmpeg.h"

//----------------------------------------------------------------------------
// FFmpegNotFoundDialog
//----------------------------------------------------------------------------

FFmpegNotFoundDialog::FFmpegNotFoundDialog(wxWindow *parent)
   :  wxDialogWrapper(parent, wxID_ANY, XO("FFmpeg not found"))
{
   // Handle OK button
   Bind(
      wxEVT_BUTTON,
      [this](wxCommandEvent&) {
         if (mDontShow->GetValue())
         {
            FFmpegNotFoundDontShow.Write(true);
            gPrefs->Flush();
         }
         EndModal(0);
      },
      wxID_OK
   );

   SetName();
   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);
}

void FFmpegNotFoundDialog::PopulateOrExchange(ShuttleGui & S)
{
   wxString text;

   S.SetBorder(10);
   S.StartVerticalLay(true);
   {
      S.AddFixedText(XO(
"Tenacity attempted to use FFmpeg to import an audio file,\n"
"but the libraries were not found.\n\n"
"To use FFmpeg import, go to Edit > Preferences > Libraries\n"
"to download or locate the FFmpeg libraries."
      ));

      mDontShow = S
         .AddCheckBox(XXO("Do not show this warning again"),
            FFmpegNotFoundDontShow.Read() );

      S.AddStandardButtons(eOkButton);
   }
   S.EndVerticalLay();

   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}
