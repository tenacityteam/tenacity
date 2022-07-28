/**********************************************************************

Audacity: A Digital Audio Editor

ProjectWindowBase.h

Paul Licameli split from ProjectWindow.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_WINDOW_BASE__
#define __AUDACITY_PROJECT_WINDOW_BASE__

#include <wx/frame.h> // to inherit

class SaucedacityProject;

///\brief A top-level window associated with a project
class ProjectWindowBase /* not final */ : public wxFrame
{
public:
   explicit ProjectWindowBase(
      wxWindow * parent, wxWindowID id,
      const wxPoint & pos, const wxSize &size,
      SaucedacityProject &project );

   ~ProjectWindowBase() override;

   SaucedacityProject &GetProject() { return mProject; }
   const SaucedacityProject &GetProject() const { return mProject; }

protected:
   SaucedacityProject &mProject;
};

SAUCEDACITY_DLL_API SaucedacityProject *FindProjectFromWindow( wxWindow *pWindow );
const SaucedacityProject *FindProjectFromWindow( const wxWindow *pWindow );

#endif

