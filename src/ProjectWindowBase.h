/**********************************************************************

Audacity: A Digital Audio Editor

ProjectWindowBase.h

Paul Licameli split from ProjectWindow.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_WINDOW_BASE__
#define __AUDACITY_PROJECT_WINDOW_BASE__

#include <wx/frame.h> // to inherit

class TenacityProject;

///\brief A top-level window associated with a project
class ProjectWindowBase /* not final */ : public wxFrame
{
public:
   explicit ProjectWindowBase(
      wxWindow * parent, wxWindowID id,
      const wxPoint & pos, const wxSize &size,
      TenacityProject &project );

   ~ProjectWindowBase() override;

   TenacityProject &GetProject() { return mProject; }
   const TenacityProject &GetProject() const { return mProject; }

protected:
   TenacityProject &mProject;
};

TENACITY_DLL_API TenacityProject *FindProjectFromWindow( wxWindow *pWindow );
const TenacityProject *FindProjectFromWindow( const wxWindow *pWindow );

#endif

