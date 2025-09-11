/**********************************************************************

  Audacity: A Digital Audio Editor

  @file DropTarget.cpp
  @brief Inject drag-and-drop importation of files

  Paul Licameli split from ProjectManager.cpp

**********************************************************************/

#include <wx/dataobj.h>
#include <wx/dnd.h>

#include "AudacityException.h"
#include "FileNames.h"
#include "Project.h"
#include "ProjectFileManager.h"
#include "TrackPanel.h"

#if wxUSE_DRAG_AND_DROP
class FileObject final : public wxFileDataObject
{
public:
   FileObject()
   {
   }

   bool IsSupportedFormat(const wxDataFormat & format, Direction WXUNUSED(dir = Get)) const
      // PRL:  This function does NOT override any inherited virtual!  What does it do?
   {
      if (format.GetType() == wxDF_FILENAME) {
         return true;
      }

      return false;
   }
};

class DropTarget final : public wxFileDropTarget
{
public:
   DropTarget(AudacityProject *proj)
   {
      mProject = proj;

      // SetDataObject takes ownership
      SetDataObject(safenew FileObject());
   }

   ~DropTarget()
   {
   }

#if defined(__WXMAC__)

   bool OnDrop(wxCoord x, wxCoord y) override
   {
      // bool foundSupported = false;
      return CurrentDragHasSupportedFormat();
   }

#endif

   bool OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames) override
   {
      // Experiment shows that this function can be reached while there is no
      // catch block above in wxWidgets.  So stop all exceptions here.
      return GuardedCall<bool>(
         [&] { return ProjectFileManager::Get(*mProject).Import(filenames); });
   }

private:
   AudacityProject *mProject;
};

// Hook the construction of projects
static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject &project) {
      // We can import now, so become a drag target
      //   SetDropTarget(safenew AudacityDropTarget(this));
      //   mTrackPanel->SetDropTarget(safenew AudacityDropTarget(this));

      TrackPanel::Get( project )
         .SetDropTarget(
            // SetDropTarget takes ownership
            safenew DropTarget( &project ) );
      return nullptr;
   }
};
#endif
