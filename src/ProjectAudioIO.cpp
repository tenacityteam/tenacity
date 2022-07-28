/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioIO.cpp

Paul Licameli split from SaucedacityProject.cpp

**********************************************************************/

#include "ProjectAudioIO.h"

#include "AudioIOBase.h"
#include "Project.h"
#include "widgets/MeterPanelBase.h"

static const SaucedacityProject::AttachedObjects::RegisteredFactory sAudioIOKey{
  []( SaucedacityProject &parent ){
     return std::make_shared< ProjectAudioIO >( parent );
   }
};

ProjectAudioIO &ProjectAudioIO::Get( SaucedacityProject &project )
{
   return project.AttachedObjects::Get< ProjectAudioIO >( sAudioIOKey );
}

const ProjectAudioIO &ProjectAudioIO::Get( const SaucedacityProject &project )
{
   return Get( const_cast<SaucedacityProject &>(project) );
}

ProjectAudioIO::ProjectAudioIO( SaucedacityProject &project )
: mProject{ project }
{
}

ProjectAudioIO::~ProjectAudioIO()
{
}

int ProjectAudioIO::GetAudioIOToken() const
{
   return mAudioIOToken;
}

void ProjectAudioIO::SetAudioIOToken(int token)
{
   mAudioIOToken = token;
}

bool ProjectAudioIO::IsAudioActive() const
{
   auto gAudioIO = AudioIOBase::Get();
   return GetAudioIOToken() > 0 &&
      gAudioIO->IsStreamActive(GetAudioIOToken());
}

MeterPanelBase *ProjectAudioIO::GetPlaybackMeter()
{
   return mPlaybackMeter;
}

void ProjectAudioIO::SetPlaybackMeter(MeterPanelBase *playback)
{
   auto &project = mProject;
   mPlaybackMeter = playback;
   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO)
   {
      gAudioIO->SetPlaybackMeter( &project , mPlaybackMeter );
   }
}

MeterPanelBase *ProjectAudioIO::GetCaptureMeter()
{
   return mCaptureMeter;
}

void ProjectAudioIO::SetCaptureMeter(MeterPanelBase *capture)
{
   auto &project = mProject;
   mCaptureMeter = capture;

   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO)
   {
      gAudioIO->SetCaptureMeter( &project, mCaptureMeter );
   }
}
