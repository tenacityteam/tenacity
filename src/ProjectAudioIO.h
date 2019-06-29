/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioIO.h

Paul Licameli split from TenacityProject.h

**********************************************************************/

#ifndef __PROJECT_AUDIO_IO__
#define __PROJECT_AUDIO_IO__

#include "ClientData.h" // to inherit
#include <wx/weakref.h>

#include <memory>
class TenacityProject;
class Meter;

///\ brief Holds per-project state needed for interaction with AudioIO,
/// including the audio stream token and pointers to meters
class TENACITY_DLL_API ProjectAudioIO final
   : public ClientData::Base
{
public:
   static ProjectAudioIO &Get( TenacityProject &project );
   static const ProjectAudioIO &Get( const TenacityProject &project );

   explicit ProjectAudioIO( TenacityProject &project );
   ProjectAudioIO( const ProjectAudioIO & ) = delete;
   ProjectAudioIO &operator=( const ProjectAudioIO & ) = delete;
   ~ProjectAudioIO();

   int GetAudioIOToken() const;
   bool IsAudioActive() const;
   void SetAudioIOToken(int token);

   const std::shared_ptr<Meter> &GetPlaybackMeter() const;
   void SetPlaybackMeter(
      const std::shared_ptr<Meter> &playback);
   const std::shared_ptr<Meter> &GetCaptureMeter() const;
   void SetCaptureMeter(
      const std::shared_ptr<Meter> &capture);

private:
   TenacityProject &mProject;

   // Project owned meters
   std::shared_ptr<Meter> mPlaybackMeter;
   std::shared_ptr<Meter> mCaptureMeter;

   int  mAudioIOToken{ -1 };
};

#endif
