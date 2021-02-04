/*!********************************************************************

  Audacity: A Digital Audio Editor

  @file ProjectWindows.h
  @brief accessors for certain important windows associated with each project

  Paul Licameli split from Project.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_WINDOWS__
#define __AUDACITY_PROJECT_WINDOWS__

#include "ClientData.h"

class TenacityProject;
class wxFrame;
class wxWindow;
namespace GenericUI { class WindowPlacement; }

///\brief Get the top-level window associated with the project (as a wxFrame
/// only, when you do not need to use the subclass ProjectWindow)
TENACITY_DLL_API wxFrame &GetProjectFrame( TenacityProject &project );
TENACITY_DLL_API const wxFrame &GetProjectFrame( const TenacityProject &project );

///\brief Get a pointer to the window associated with a project, or null if
/// the given pointer is null, or the window was not yet set.
TENACITY_DLL_API wxFrame *FindProjectFrame( TenacityProject *project );

///\brief Get a pointer to the window associated with a project, or null if
/// the given pointer is null, or the window was not yet set.
TENACITY_DLL_API const wxFrame *FindProjectFrame( const TenacityProject *project );

//! Make a WindowPlacement object suitable for `project` (which may be null)
/*! @post return value is not null */
TENACITY_DLL_API std::unique_ptr<const GenericUI::WindowPlacement>
ProjectFramePlacement( TenacityProject *project );

///\brief Get the main sub-window of the project frame that displays track data
// (as a wxWindow only, when you do not need to use the subclass TrackPanel)
TENACITY_DLL_API wxWindow &GetProjectPanel( TenacityProject &project );
TENACITY_DLL_API const wxWindow &GetProjectPanel(
   const TenacityProject &project );

TENACITY_DLL_API void SetProjectPanel(
   TenacityProject &project, wxWindow &panel );
TENACITY_DLL_API void SetProjectFrame(
   TenacityProject &project, wxFrame &frame );

// Container of pointers to various windows associated with the project, which
// is not responsible for destroying them -- wxWidgets handles that instead
class AttachedWindows : public ClientData::Site<
   AttachedWindows, wxWindow, ClientData::SkipCopying, ClientData::BarePtr
>
{
public:
   explicit AttachedWindows(TenacityProject &project)
      : mProject{project} {}
   operator TenacityProject & () const { return mProject; }
private:
   TenacityProject &mProject;
};

TENACITY_DLL_API AttachedWindows &GetAttachedWindows(TenacityProject &project);

#endif
