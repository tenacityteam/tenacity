/*!********************************************************************

  Audacity: A Digital Audio Editor

  @file ProjectWindows.cpp

  Paul Licameli split from Project.cpp

**********************************************************************/

#include "ProjectWindows.h"
#include "Project.h"
#include "widgets/wxWidgetsBasicUI.h"

#include <wx/frame.h>

namespace {
struct ProjectWindows final : ClientData::Base
{
   static ProjectWindows &Get( TenacityProject &project );
   static const ProjectWindows &Get( const TenacityProject &project );
   explicit ProjectWindows(TenacityProject &project)
      : mAttachedWindows{project}
   {}

   wxWeakRef< wxWindow > mPanel{};
   wxWeakRef< wxFrame > mFrame{};

   AttachedWindows mAttachedWindows;
};

const TenacityProject::AttachedObjects::RegisteredFactory key{
   [](TenacityProject &project) {
      return std::make_unique<ProjectWindows>(project);
   }
};

ProjectWindows &ProjectWindows::Get( TenacityProject &project )
{
   return project.AttachedObjects::Get< ProjectWindows >( key );
}

const ProjectWindows &ProjectWindows::Get( const TenacityProject &project )
{
   return Get( const_cast< TenacityProject & >( project ) );
}
}

TENACITY_DLL_API wxWindow &GetProjectPanel( TenacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mPanel;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

TENACITY_DLL_API const wxWindow &GetProjectPanel(
   const TenacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mPanel;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

TENACITY_DLL_API void SetProjectPanel(
   TenacityProject &project, wxWindow &panel )
{
   ProjectWindows::Get(project).mPanel = &panel;
}

TENACITY_DLL_API wxFrame &GetProjectFrame( TenacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mFrame;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

TENACITY_DLL_API const wxFrame &GetProjectFrame( const TenacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mFrame;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

wxFrame *FindProjectFrame( TenacityProject *project ) {
   if (!project)
      return nullptr;
   return ProjectWindows::Get(*project).mFrame;
}

const wxFrame *FindProjectFrame( const TenacityProject *project ) {
   if (!project)
      return nullptr;
   return ProjectWindows::Get(*project).mFrame;
}

std::unique_ptr<const GenericUI::WindowPlacement>
ProjectFramePlacement( TenacityProject *project )
{
   if (!project)
      return std::make_unique<GenericUI::WindowPlacement>();
   return std::make_unique<wxWidgetsWindowPlacement>(
      &GetProjectFrame(*project));
}

void SetProjectFrame(TenacityProject &project, wxFrame &frame )
{
   ProjectWindows::Get(project).mFrame = &frame;
}

TENACITY_DLL_API AttachedWindows &GetAttachedWindows(TenacityProject &project)
{
   return ProjectWindows::Get(project).mAttachedWindows;
}
