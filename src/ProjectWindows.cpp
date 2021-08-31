/**********************************************************************

  Audacity: A Digital Audio Editor

  Project.cpp

  Dominic Mazzoni
  Vaughan Johnson

*//*******************************************************************/


#include "Project.h"

#include "widgets/wxWidgetsBasicUI.h"

#include <wx/display.h>
#include <wx/filename.h>
#include <wx/frame.h>

wxDEFINE_EVENT(EVT_TRACK_PANEL_TIMER, wxCommandEvent);

size_t AllProjects::size() const
{
   return gTenacityProjects.size();
}

auto AllProjects::begin() const -> const_iterator
{
   return gTenacityProjects.begin();
}

auto AllProjects::end() const -> const_iterator
{
   return gTenacityProjects.end();
}

auto AllProjects::rbegin() const -> const_reverse_iterator
{
   return gTenacityProjects.rbegin();
}

auto AllProjects::rend() const -> const_reverse_iterator
{
   return gTenacityProjects.rend();
}

auto AllProjects::Remove( TenacityProject &project ) -> value_type
{
   std::lock_guard<std::mutex> guard{ Mutex() };
   auto start = begin(), finish = end(), iter = std::find_if(
      start, finish,
      [&]( const value_type &ptr ){ return ptr.get() == &project; }
   );
   if (iter == finish)
      return nullptr;
   auto result = *iter;
   gTenacityProjects.erase( iter );
   return result;
}

void AllProjects::Add( const value_type &pProject )
{
   if (!pProject) {
      wxASSERT(false);
      return;
   }
   std::lock_guard<std::mutex> guard{ Mutex() };
   gTenacityProjects.push_back( pProject );
}

std::mutex &AllProjects::Mutex()
{
   static std::mutex theMutex;
   return theMutex;
}

int TenacityProject::mProjectCounter=0;// global counter.

/* Define Global Variables */
//This array holds onto all of the projects currently open
AllProjects::Container AllProjects::gTenacityProjects;

TenacityProject::TenacityProject()
{
   mProjectNo = mProjectCounter++; // Bug 322
   AttachedObjects::BuildAll();
   // But not for the attached windows.  They get built only on demand, such as
   // from menu items.
}

TenacityProject::~TenacityProject()
{
}

void TenacityProject::SetFrame( wxFrame *pFrame )
{
   mFrame = pFrame;
}

void TenacityProject::SetPanel( wxWindow *pPanel )
{
   mPanel = pPanel;
}

const wxString &TenacityProject::GetProjectName() const
{
   return mName;
}

void TenacityProject::SetProjectName(const wxString &name)
{
   mName = name;
}

FilePath TenacityProject::GetInitialImportPath() const
{
   return mInitialImportPath;
}

void TenacityProject::SetInitialImportPath(const FilePath &path)
{
   if (mInitialImportPath.empty())
   {
      mInitialImportPath = path;
   }
}

TENACITY_DLL_API wxFrame &GetProjectFrame( TenacityProject &project )
{
   auto ptr = project.GetFrame();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

TENACITY_DLL_API const wxFrame &GetProjectFrame( const TenacityProject &project )
{
   auto ptr = project.GetFrame();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

std::unique_ptr<const BasicUI::WindowPlacement>
ProjectFramePlacement( TenacityProject *project )
{
   if (!project)
      return std::make_unique<BasicUI::WindowPlacement>();
   return std::make_unique<wxWidgetsWindowPlacement>(
      &GetProjectFrame(*project));
}

TENACITY_DLL_API wxWindow &GetProjectPanel( TenacityProject &project )
{
   auto ptr = project.GetPanel();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

TENACITY_DLL_API const wxWindow &GetProjectPanel(
   const TenacityProject &project )
{
   auto ptr = project.GetPanel();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

// Generate the needed, linkable registry functions
DEFINE_XML_METHOD_REGISTRY( ProjectFileIORegistry );
