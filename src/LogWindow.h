/**********************************************************************

Audacity: A Digital Audio Editor

LogWindow.h

Paul Licameli split from TenacityLogger.h

**********************************************************************/

#ifndef __AUDACITY_LOG_WINDOW__
#define __AUDACITY_LOG_WINDOW__

//! Maintains the unique logging window which displays debug information
class SAUCEDACITY_DLL_API LogWindow
{
public:
   //! Show or hide the unique logging window; create it on demand the first time it is shown
   static void Show(bool show = true);
};

#endif
