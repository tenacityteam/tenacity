/**********************************************************************

  Audacity: A Digital Audio Editor

  VSTControlOSX.h

  Leland Lucius

**********************************************************************/

#ifndef AUDACITY_VSTCONTROLOSX_H
#define AUDACITY_VSTCONTROLOSX_H

#include <wx/osx/core/private.h>
#include <wx/osx/cocoa/private.h>

#include "VSTControl.h"

class VSTControlImpl final : public wxWidgetCocoaImpl
{
public :
   VSTControlImpl(wxWindowMac *peer, NSView *view);
   ~VSTControlImpl();
};

class VSTControl : public VSTControlBase
{
public:
   VSTControl();
   ~VSTControl();

   bool Create(wxWindow *parent, VSTEffectLink *link);
   void Close();

private:
   void CreateCocoa();

private:
   NSView *mVSTView;
   NSView *mView;
};

#endif
