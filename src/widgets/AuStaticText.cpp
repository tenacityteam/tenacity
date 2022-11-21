/**********************************************************************

  Tenacity: A Digital Audio Editor

  AuStaticText.cpp

  Avery King split from Theme.cpp

  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#include "AuStaticText.h"
#include "../theme/AllThemeResources.h"
#include "../theme/Theme.h"

BEGIN_EVENT_TABLE(AuStaticText, wxWindow)
    EVT_PAINT(AuStaticText::OnPaint)
    EVT_ERASE_BACKGROUND(AuStaticText::OnErase)
END_EVENT_TABLE()
 
AuStaticText::AuStaticText(wxWindow* parent, wxString textIn) :
 wxWindow(parent, wxID_ANY)
{
   int textWidth, textHeight;

   int fontSize = 11;
   #ifdef __WXMSW__
      fontSize = 9;
   #endif
   wxFont font(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
   GetTextExtent(textIn, &textWidth, &textHeight, NULL, NULL, &font);

   SetFont( font );
   SetMinSize( wxSize(textWidth, textHeight) );
   SetBackgroundColour( theTheme.Colour( clrMedium));
   SetForegroundColour( theTheme.Colour( clrTrackPanelText));
   SetName(textIn);
   SetLabel(textIn);
}
 
void AuStaticText::OnPaint(wxPaintEvent&)
{
   wxPaintDC dc(this);
   //dc.SetTextForeground( theTheme.Colour( clrTrackPanelText));
   dc.Clear();
   dc.DrawText( GetLabel(), 0,0);
}
