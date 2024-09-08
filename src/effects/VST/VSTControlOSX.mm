/**********************************************************************

  Audacity: A Digital Audio Editor

  VSTControlOSX.mm

  Leland Lucius

  Several ideas and code snippets taken from HairerSoft's HSVSTView class:

      http://www.hairersoft.com/Downloads/HSVSTView.zip

      Created by Martin on 02/06/2007.
      Copyright 2010 by HairerSoft.
      
      You are most welcome to use this code in your own (open source or not)
      project. Use at your own risk of course, etc. Please acknowledge at an
      appropriate location (manual or about box for example).
      
      Bug reports most welcome: Martin@HairerSoft.com
      
**********************************************************************/


#include "VSTControlOSX.h"

#include <memory>

@interface VSTView : NSView
{
}
@end

@implementation VSTView
 
+ (void)initialize
{
   static BOOL initialized = NO;
   if (!initialized)
   {
      initialized = YES;
      wxOSXCocoaClassAddWXMethods(self);
   }
}
@end

VSTControlImpl::VSTControlImpl(wxWindowMac *peer, NSView *view)
:  wxWidgetCocoaImpl(peer, view)
{
}

VSTControlImpl::~VSTControlImpl()
{
}

VSTControl::VSTControl()
:  VSTControlBase()
{
   mVSTView = nil;
   mView = nil;
}

VSTControl::~VSTControl()
{
   Close();
}

void VSTControl::Close()
{
}

bool VSTControl::Create(wxWindow *parent, VSTEffectLink *link)
{
   DontCreatePeer();
   
   if (!VSTControlBase::Create(parent, link))
   {
      return false;
   }

   mVSTView = [VSTView alloc];
   if (!mVSTView)
   {
      return false;
   }
   [mVSTView init];
   [mVSTView retain];

   // wxWidgets takes ownership so safenew
   SetPeer(safenew VSTControlImpl(this, mVSTView));

   CreateCocoa();

   if (!mView)
   {
      return false;
   }

   // Must get the size again since SetPeer() could cause it to change
   SetInitialSize(GetMinSize());

   MacPostControlCreate(wxDefaultPosition, wxDefaultSize);

   return true;
}

void VSTControl::CreateCocoa()
{
   if ((mLink->callDispatcher(effCanDo, 0, 0, (void *) "hasCockosViewAsConfig", 0.0) & 0xffff0000) != 0xbeef0000)
   {
      return;
   }

   VstRect *rect;

   // Some effects like to have us get their rect before opening them.
   mLink->callDispatcher(effEditGetRect, 0, 0, &rect, 0.0);

   // Ask the effect to add its GUI
   mLink->callDispatcher(effEditOpen, 0, 0, mVSTView, 0.0);

   // Get the subview it created
   mView = [[mVSTView subviews] objectAtIndex:0];
   if (mView == NULL)
   {
      // Doesn't seem the effect created the subview.  This can
      // happen when an effect uses the content view directly.
      // As of this time, we will not try to support those and
      // just fall back to the textual interface.
      return;
   }

   // Get the final bounds of the effect GUI
   mLink->callDispatcher(effEditGetRect, 0, 0, &rect, 0.0);

   NSRect frame = {
      { 0, 0 },
      { (CGFloat) rect->right - rect->left, (CGFloat) rect->bottom - rect->top }
   };

   [mView setFrame:frame];

   [mVSTView addSubview:mView];

   SetMinSize(wxSize(frame.size.width, frame.size.height));

   return;
}
