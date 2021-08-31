/*!********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file ActiveProject.cpp
 
 Paul Licameli split from Project.cpp
 
 **********************************************************************/

#include "ActiveProject.h"
#include "KeyboardCapture.h"
#include "Project.h"

#include <wx/app.h>
#include <wx/frame.h>

wxDEFINE_EVENT(EVT_PROJECT_ACTIVATION, wxCommandEvent);

//This is a pointer to the currently-active project.
static TenacityProject *gActiveProject;

TENACITY_DLL_API TenacityProject *GetActiveProject()
{
   return gActiveProject;
}

void SetActiveProject(TenacityProject * project)
{
   if ( gActiveProject != project ) {
      gActiveProject = project;
      KeyboardCapture::Capture( nullptr );
      wxTheApp->QueueEvent( safenew wxCommandEvent{ EVT_PROJECT_ACTIVATION } );
   }
   wxTheApp->SetTopWindow( FindProjectFrame( project ) );
}
