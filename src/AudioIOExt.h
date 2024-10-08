/*!********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file AudioIOExt.h
 @brief Abstract base class for hooks into audio playback procedures
 
 Paul Licameli
 
 **********************************************************************/

#ifndef __AUDACITY_AUDIO_IO_EXT__
#define __AUDACITY_AUDIO_IO_EXT__

#include <functional>
#include <memory>
#include <vector>

// Tenacity libraries
#include <lib-audio-devices/AudioIOBase.h>

struct PaStreamCallbackTimeInfo;
struct PaStreamInfo;
struct PlaybackSchedule;
struct TransportTracks;

class TENACITY_DLL_API AudioIOExt : public AudioIOExtBase
{
public:
   using Factory = std::function<
      std::unique_ptr<AudioIOExt>( const PlaybackSchedule& ) >;
   using Factories = std::vector<AudioIOExt::Factory>;
   static Factories &GetFactories();

   //! Typically statically constructed
   struct TENACITY_DLL_API RegisteredFactory{
      explicit RegisteredFactory(Factory factory);
      ~RegisteredFactory();
   };

   virtual ~AudioIOExt();

   // Formerly in AudioIoCallback
   virtual void ComputeOtherTimings(double rate,
      const PaStreamCallbackTimeInfo *timeInfo,
      unsigned long framesPerBuffer) = 0;
   virtual void SignalOtherCompletion() = 0;
   virtual unsigned CountOtherSoloTracks() const = 0;

   // Formerly in AudioIO
   virtual bool StartOtherStream(const TransportTracks &tracks,
      const PaStreamInfo* info, double startTime, double rate) = 0;
   virtual void AbortOtherStream() = 0;
   virtual void FillOtherBuffers(
      double rate, unsigned long pauseFrames, bool paused, bool hasSolo) = 0;
   virtual void StopOtherStream() = 0;
};

#endif
