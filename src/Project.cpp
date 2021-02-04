/**********************************************************************

  Audacity: A Digital Audio Editor

  Project.cpp

  Dominic Mazzoni
  Vaughan Johnson

*//*******************************************************************/


#include "Project.h"

#include <wx/display.h>
#include <wx/filename.h>

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

// Generate the needed, linkable registry functions
DEFINE_XML_METHOD_REGISTRY( ProjectFileIORegistry );
