/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIO.cpp

  Copyright 2000-2004:
  Dominic Mazzoni
  Joshua Haberman
  Markus Meyer
  Matt Brubeck

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

********************************************************************//**

\class AudioIoCallback
\brief AudioIoCallback is a class that implements the callback required 
by PortAudio.  The callback needs to be responsive, has no GUI, and 
copies data into and out of the sound card buffers.  It also sends data
to the meters.


*//*****************************************************************//**

\class AudioIO
\brief AudioIO uses the PortAudio library to play and record sound.

  Great care and attention to detail are necessary for understanding and
  modifying this system.  The code in this file is run from three
  different thread contexts: the UI thread, the disk thread (which
  this file creates and maintains; in the code, this is called the
  Audio Thread), and the PortAudio callback thread.
  To highlight this deliniation, the file is divided into three parts
  based on what thread context each function is intended to run in.

  \todo run through all functions called from audio and portaudio threads
  to verify they are thread-safe. Note that synchronization of the style:
  "A sets flag to signal B, B clears flag to acknowledge completion"
  is not thread safe in a general multiple-CPU context. For example,
  B can write to a buffer and set a completion flag. The flag write can
  occur before the buffer write due to out-of-order execution. Then A
  can see the flag and read the buffer before buffer writes complete.

*//****************************************************************//**

\class AudioIOListener
\brief Monitors record play start/stop and new sample blocks.  Has
callbacks for these events.

*//****************************************************************//**

\class AudioIOStartStreamOptions
\brief struct holding stream options, including a pointer to the 
time warp info and AudioIOListener and whether the playback is looped.

*//*******************************************************************/


#include "AudioIO.h"
#include "AudioIOExt.h"
#include "AudioIOListener.h"

#include "DeviceManager.h"

#include <string>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <optional>
#include <iostream>
#include <numeric>

#include "portaudio.h"
#ifdef PA_USE_JACK
#include "pa_jack.h"
#endif

#include <wx/app.h>
#include <wx/wxcrtvararg.h>
#include <wx/time.h>

#if defined(__WXMAC__) || defined(__WXMSW__)
#include <wx/power.h>
#endif

// Tenacity libraries
#include <lib-audio-devices/AudioMemoryManager.h>
#include <lib-basic-ui/BasicUI.h>
#include <lib-exceptions/TenacityException.h>
#include <lib-math/Resample.h>
#include <lib-preferences/Prefs.h>
#include <lib-utility/MessageBuffer.h>

#include "Meter.h"
#include "Mix.h"
#include "RingBuffer.h"
#include "Decibels.h"
#include "Project.h"
#include "DBConnection.h"
#include "ProjectFileIO.h"
#include "WaveTrack.h"

#include "effects/RealtimeEffectManager.h"
#include "widgets/AudacityMessageBox.h"
#include "QualitySettings.h"

using std::max;
using std::min;

/// @brief Converts the latency preference to samples and updates the user's
/// preference.
long AudioIoCallback::GetConvertedLatencyPreference()
{
   // Find out frames per buffer
   long latency = AudioIOLatencyDuration.Read();
   bool isMilliseconds = AudioIOLatencyUnit.Read() == L"milliseconds";

   if (isMilliseconds)
   {
      latency *= mRate / 1000;
   }

   return latency;
}

AudioIO *AudioIO::Get()
{
   return static_cast< AudioIO* >( AudioIOBase::Get() );
}

wxDEFINE_EVENT(EVT_AUDIOIO_PLAYBACK, wxCommandEvent);
wxDEFINE_EVENT(EVT_AUDIOIO_CAPTURE, wxCommandEvent);
wxDEFINE_EVENT(EVT_AUDIOIO_MONITOR, wxCommandEvent);

// static
int AudioIoCallback::mNextStreamToken = 0;
double AudioIoCallback::mCachedBestRateOut;
bool AudioIoCallback::mCachedBestRatePlaying;
bool AudioIoCallback::mCachedBestRateCapturing;

constexpr size_t TimeQueueGrainSize = 2000;

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT

#ifdef __WXGTK__
   // Might #define this for a useful thing on Linux
   #undef REALTIME_ALSA_THREAD
#else
   // never on the other operating systems
   #undef REALTIME_ALSA_THREAD
#endif

#ifdef REALTIME_ALSA_THREAD
#include "pa_linux_alsa.h"
#endif


struct AudioIoCallback::ScrubState : NonInterferingBase
{
   ScrubState(double t0,
              double rate,
              const ScrubbingOptions &options)
      : mRate(rate)
      , mStartTime( t0 )
   {
      const double t1 = options.bySpeed ? options.initSpeed : t0;
      Update( t1, options );
   }

   void Update(double end, const ScrubbingOptions &options)
   {
      // Called by another thread
      mMessage.Write({ end, options });
   }

   void Get(sampleCount &startSample, sampleCount &endSample,
         sampleCount inDuration, sampleCount &duration)
   {
      // Called by the thread that calls AudioIO::TrackBufferExchange
      startSample = endSample = duration = -1LL;
      sampleCount s0Init;

      Message message( mMessage.Read() );
      if ( !mStarted ) {
         s0Init = llrint( mRate *
            std::max( message.options.minTime,
               std::min( message.options.maxTime, mStartTime ) ) );

         // Make some initial silence. This is not needed in the case of
         // keyboard scrubbing or play-at-speed, because the initial speed
         // is known when this function is called the first time.
         if ( !(message.options.isKeyboardScrubbing ||
            message.options.isPlayingAtSpeed) ) {
            mData.mS0 = mData.mS1 = s0Init;
            mData.mGoal = -1;
            mData.mDuration = duration = inDuration;
            mData.mSilence = 0;
         }
      }

      if (mStarted || message.options.isKeyboardScrubbing ||
         message.options.isPlayingAtSpeed) {
         Data newData;
         inDuration += mAccumulatedSeekDuration;

         // If already started, use the previous end as NEW start.
         const auto s0 = mStarted ? mData.mS1 : s0Init;
         const sampleCount s1 ( message.options.bySpeed
            ? s0.as_double() +
               lrint(inDuration.as_double() * message.end) // end is a speed
            : lrint(message.end * mRate)            // end is a time
         );
         auto success =
            newData.Init(mData, s0, s1, inDuration, message.options, mRate);
         if (success)
            mAccumulatedSeekDuration = 0;
         else {
            mAccumulatedSeekDuration += inDuration;
            return;
         }
         mData = newData;
      };

      mStarted = true;

      Data &entry = mData;
      if (  mStopped.load( std::memory_order_relaxed ) ) {
         // We got the shut-down signal, or we discarded all the work.
         // Output the -1 values.
      }
      else if (entry.mDuration > 0) {
         // First use of the entry
         startSample = entry.mS0;
         endSample = entry.mS1;
         duration = entry.mDuration;
         entry.mDuration = 0;
      }
      else if (entry.mSilence > 0) {
         // Second use of the entry
         startSample = endSample = entry.mS1;
         duration = entry.mSilence;
         entry.mSilence = 0;
      }
   }

   void Stop()
   {
      mStopped.store( true, std::memory_order_relaxed );
   }

#if 0
   // Needed only for the DRAG_SCRUB experiment
   // Should make mS1 atomic then?
   double LastTrackTime() const
   {
      // Needed by the main thread sometimes
      return mData.mS1.as_double() / mRate;
   }
#endif

   ~ScrubState() {}

private:
   struct Data
   {
      Data()
         : mS0(0)
         , mS1(0)
         , mGoal(0)
         , mDuration(0)
         , mSilence(0)
      {}

      bool Init(Data &rPrevious, sampleCount s0, sampleCount s1,
         sampleCount duration,
         const ScrubbingOptions &options, double rate)
      {
         auto previous = &rPrevious;
         auto origDuration = duration;
         mSilence = 0;

         const bool &adjustStart = options.adjustStart;

         assert(duration > 0);
         double speed =
            (std::abs((s1 - s0).as_long_long())) / duration.as_double();
         bool adjustedSpeed = false;

         auto minSpeed = std::min(options.minSpeed, options.maxSpeed);
         assert(minSpeed == options.minSpeed);

         // May change the requested speed and duration
         if (!adjustStart && speed > options.maxSpeed)
         {
            // Reduce speed to the maximum selected in the user interface.
            speed = options.maxSpeed;
            mGoal = s1;
            adjustedSpeed = true;
         }
         else if (!adjustStart &&
            previous->mGoal >= 0 &&
            previous->mGoal == s1)
         {
            // In case the mouse has not moved, and playback
            // is catching up to the mouse at maximum speed,
            // continue at no less than maximum.  (Without this
            // the final catch-up can make a slow scrub interval
            // that drops the pitch and sounds wrong.)
            minSpeed = options.maxSpeed;
            mGoal = s1;
            adjustedSpeed = true;
         }
         else
            mGoal = -1;

         if (speed < minSpeed) {
            if (s0 != s1 && adjustStart)
               // Do not trim the duration.
               ;
            else
               // Trim the duration.
               duration =
                  std::max(0L, lrint(speed * duration.as_double() / minSpeed));

            speed = minSpeed;
            adjustedSpeed = true;
         }

         if (speed < ScrubbingOptions::MinAllowedScrubSpeed()) {
            // Mixers were set up to go only so slowly, not slower.
            // This will put a request for some silence in the work queue.
            adjustedSpeed = true;
            speed = 0.0;
         }

         // May change s1 or s0 to match speed change or stay in bounds of the project

         if (adjustedSpeed && !adjustStart)
         {
            // adjust s1
            const sampleCount diff = lrint(speed * duration.as_double());
            if (s0 < s1)
               s1 = s0 + diff;
            else
               s1 = s0 - diff;
         }

         bool silent = false;

         // Adjust s1 (again), and duration, if s1 is out of bounds,
         // or abandon if a stutter is too short.
         // (Assume s0 is in bounds, because it equals the last scrub's s1 which was checked.)
         if (s1 != s0)
         {
            // When playback follows a fast mouse movement by "stuttering"
            // at maximum playback, don't make stutters too short to be useful.
            if (options.adjustStart &&
                duration < llrint( options.minStutterTime * rate ) )
               return false;

            sampleCount minSample { llrint(options.minTime * rate) };
            sampleCount maxSample { llrint(options.maxTime * rate) };
            auto newDuration = duration;
            const auto newS1 = std::max(minSample, std::min(maxSample, s1));
            if(s1 != newS1)
               newDuration = std::max( sampleCount{ 0 },
                  sampleCount(
                     duration.as_double() * (newS1 - s0).as_double() /
                        (s1 - s0).as_double()
                  )
               );
            if (newDuration == 0) {
               // A silent scrub with s0 == s1
               silent = true;
               s1 = s0;
            }
            else if (s1 != newS1) {
               // Shorten
               duration = newDuration;
               s1 = newS1;
            }
         }

         if (adjustStart && !silent)
         {
            // Limit diff because this is seeking.
            const sampleCount diff =
               lrint(std::min(options.maxSpeed, speed) * duration.as_double());
            if (s0 < s1)
               s0 = s1 - diff;
            else
               s0 = s1 + diff;
         }

         mS0 = s0;
         mS1 = s1;
         mDuration = duration;
         if (duration < origDuration)
            mSilence = origDuration - duration;

         return true;
      }

      sampleCount mS0;
      sampleCount mS1;
      sampleCount mGoal;
      sampleCount mDuration;
      sampleCount mSilence;
   };

   double mStartTime;
   bool mStarted{ false };
   std::atomic<bool> mStopped { false };
   Data mData;
   const double mRate;
   struct Message {
      Message() = default;
      Message(const Message&) = default;
      double end;
      ScrubbingOptions options;
   };
   MessageBuffer<Message> mMessage;
   sampleCount mAccumulatedSeekDuration{};
};
#endif

int audacityAudioCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData );

void StartAudioIOThread()
{
   AudioIO *gAudioIO;
   while( (gAudioIO = AudioIO::Get()) != nullptr )
   {
      using Clock = std::chrono::steady_clock;
      auto loopPassStart = Clock::now();
      const auto interval = ScrubPollInterval_ms;

      // Set LoopActive outside the tests to avoid race condition
      gAudioIO->mAudioThreadTrackBufferExchangeLoopActive = true;
      if( gAudioIO->mAudioThreadShouldCallTrackBufferExchangeOnce )
      {
         gAudioIO->TrackBufferExchange();
         gAudioIO->mAudioThreadShouldCallTrackBufferExchangeOnce = false;
      }
      else if( gAudioIO->mAudioThreadTrackBufferExchangeLoopRunning )
      {
         gAudioIO->TrackBufferExchange();
      }
      gAudioIO->mAudioThreadTrackBufferExchangeLoopActive = false;

      if ( gAudioIO->mPlaybackSchedule.Interactive() )
      {
         std::this_thread::sleep_until(
            loopPassStart + std::chrono::milliseconds( interval ) );
      } else
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
   }
}

//////////////////////////////////////////////////////////////////////
//
//     UI Thread Context
//
//////////////////////////////////////////////////////////////////////

void AudioIO::Init()
{
   ugAudioIO.reset(safenew AudioIO());

   // Start the audio (and MIDI) IO threads
   std::thread audioThread(StartAudioIOThread);
   audioThread.detach();

   // Make sure device prefs are initialized
   if (gPrefs->Read(wxT("AudioIO/RecordingDevice"), wxT("")).empty()) {
      int i = getRecordDevIndex();
      const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
      if (info) {
         AudioIORecordingDevice.Write(DeviceName(info));
         AudioIOHost.Write(HostName(info));
      }
   }

   if (gPrefs->Read(wxT("AudioIO/PlaybackDevice"), wxT("")).empty()) {
      int i = getPlayDevIndex();
      const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
      if (info) {
         AudioIOPlaybackDevice.Write(DeviceName(info));
         AudioIOHost.Write(HostName(info));
      }
   }

   gPrefs->Flush();
}

void AudioIO::Deinit()
{
   ugAudioIO.reset();
}

bool AudioIO::ValidateDeviceNames(const std::string &play, const std::string &rec)
{
   const PaDeviceInfo *pInfo = Pa_GetDeviceInfo(getPlayDevIndex(play));
   const PaDeviceInfo *rInfo = Pa_GetDeviceInfo(getRecordDevIndex(rec));

   // Valid iff both defined and the same api.
   return pInfo != nullptr && rInfo != nullptr && pInfo->hostApi == rInfo->hostApi;
}

AudioIO::AudioIO()
{
   if (!std::atomic<double>{}.is_lock_free()) {
      // If this check fails, then the atomic<double> members in AudioIO.h
      // might be changed to atomic<float> to be more efficient with some
      // loss of precision.  That could be conditionally compiled depending
      // on the platform.
      throw std::runtime_error("atomic<double> could be changed to atomic<float>, reducing precision");
   }

   // This exception is thrown because of casting in the callback 
   // functions where we cast a tempFloats buffer to a (short*) buffer.
   // We assert this at compile time, although we previously asserted in the
   // GUI (afterwards throwing exceptions).
   static_assert(sizeof(short) <= sizeof(float), "sizeof(short) is not less than sizeof(float)");

   mAudioThreadShouldCallTrackBufferExchangeOnce = false;
   mAudioThreadTrackBufferExchangeLoopRunning = false;
   mAudioThreadTrackBufferExchangeLoopActive = false;
   mPortStreamV19 = NULL;

   mNumPauseFrames = 0;

   mStreamToken = 0;

   mLastPaError = paNoError;

   mLastRecordingOffset = 0.0;
   mNumCaptureChannels = 0;
   mPaused = false;
   mSilenceLevel = 0.0;

   mUpdateMeters = false;
   mUpdatingMeters = false;

   mOutputMeter.reset();

   mBuffersPrepared = false;

#ifdef PA_USE_JACK
   // Set JACK client name
   {
      PaError error = PaJack_SetClientName("Tenacity");
      if (error != paNoError)
      {
         wxLogWarning("Failed to set JACK client name");
      }
   }
#endif

   PaError err = Pa_Initialize();

   if (err != paNoError) {
      auto errStr = XO("Could not find any audio devices.\n");
      errStr += XO("You will not be able to play or record audio.\n\n");
      std::string paErrStr = std::string(Pa_GetErrorText(err)); // ANERRUPTION: UTF-8?
      if (!paErrStr.empty())
      {
         errStr += XO("Error: %s").Format( paErrStr );
      }

      throw SimpleMessageBoxException(ExceptionType::Internal, errStr, XO("PortAudio Error"));

      // Since PortAudio is not initialized, all calls to PortAudio
      // functions will fail.  This will give reasonable behavior, since
      // the user will be able to do things not relating to audio i/o,
      // but any attempt to play or record will simply fail.
   }

   mLastPlaybackTimeMillis = 0;

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   mScrubState = NULL;
   mScrubDuration = 0;
   mSilentScrub = false;
#endif
}

AudioIO::~AudioIO()
{

   // FIXME: ? TRAP_ERR.  Pa_Terminate probably OK if err without reporting.
   Pa_Terminate();

   // This causes reentrancy issues during application shutdown
   // wxTheApp->Yield();
}

static PaSampleFormat AudacityToPortAudioSampleFormat(sampleFormat format)
{
   switch(format) {
   case int16Sample:
      return paInt16;
   case int24Sample:
      return paInt24;
   case floatSample:
   default:
      return paFloat32;
   }
}

bool AudioIO::StartPortAudioStream(const AudioIOStartStreamOptions &options,
                                   unsigned int numPlaybackChannels,
                                   unsigned int numCaptureChannels,
                                   sampleFormat captureFormat)
{
   auto sampleRate = options.rate;
   mNumPauseFrames = 0;
   mOwningProject = options.pProject;

   // PRL:  Protection from crash reported by David Bailes, involving starting
   // and stopping with frequent changes of active window, hard to reproduce
   if (mOwningProject.expired())
      return false;

   mInputMeter.reset();
   mOutputMeter.reset();

   mLastPaError = paNoError;
   // pick a rate to do the audio I/O at, from those available. The project
   // rate is suggested, but we may get something else if it isn't supported
   mRate = GetBestRate(numCaptureChannels > 0, numPlaybackChannels > 0, sampleRate);

   // July 2016 (Carsten and Uwe)
   // BUG 193: Tell PortAudio sound card will handle 24 bit (under DirectSound) using 
   // userData.
   int captureFormat_saved = captureFormat;
   // Special case: Our 24-bit sample format is different from PortAudio's
   // 3-byte packed format. So just make PortAudio return float samples,
   // since we need float values anyway to apply the gain.
   // ANSWER-ME: So we *never* actually handle 24-bit?! This causes mCapture to 
   // be set to floatSample below.
   // JKC: YES that's right.  Internally Audacity uses float, and float has space for
   // 24 bits as well as exponent.  Actual 24 bit would require packing and
   // unpacking unaligned bytes and would be inefficient.
   // ANSWER ME: is floatSample 64 bit on 64 bit machines?
   if (captureFormat == int24Sample)
      captureFormat = floatSample;

   mNumPlaybackChannels = numPlaybackChannels;
   mNumCaptureChannels = numCaptureChannels;

   bool usePlayback = false, useCapture = false;
   PaStreamParameters playbackParameters{};
   PaStreamParameters captureParameters{};

   if( numPlaybackChannels > 0)
   {
      usePlayback = true;

      // this sets the device index to whatever is "right" based on preferences,
      // then defaults
      playbackParameters.device = getPlayDevIndex();

      const PaDeviceInfo *playbackDeviceInfo;
      playbackDeviceInfo = Pa_GetDeviceInfo( playbackParameters.device );

      if( playbackDeviceInfo == NULL )
         return false;

      // regardless of source formats, we always mix to float
      playbackParameters.sampleFormat = paFloat32;
      playbackParameters.hostApiSpecificStreamInfo = NULL;
      playbackParameters.channelCount = mNumPlaybackChannels;

      playbackParameters.suggestedLatency = mSoftwarePlaythrough ?
         playbackDeviceInfo->defaultLowOutputLatency :
         0.0;

      mOutputMeter = options.playbackMeter;
   }

   if( numCaptureChannels > 0)
   {
      useCapture = true;
      mCaptureFormat = captureFormat;

      const PaDeviceInfo *captureDeviceInfo;
      // retrieve the index of the device set in the prefs, or a sensible
      // default if it isn't set/valid
      captureParameters.device = getRecordDevIndex();

      captureDeviceInfo = Pa_GetDeviceInfo( captureParameters.device );

      if( captureDeviceInfo == NULL )
         return false;

      captureParameters.sampleFormat =
         AudacityToPortAudioSampleFormat(mCaptureFormat);

      captureParameters.hostApiSpecificStreamInfo = NULL;
      captureParameters.channelCount = mNumCaptureChannels;
      captureParameters.suggestedLatency = mSoftwarePlaythrough ?
         captureDeviceInfo->defaultHighInputLatency :
         0.0;

      SetCaptureMeter( mOwningProject.lock(), options.captureMeter );
   }

   SetMeters();

   // July 2016 (Carsten and Uwe)
   // BUG 193: Possibly tell portAudio to use 24 bit with DirectSound. 
   int  userData = 24;
   int* lpUserData = (captureFormat_saved == int24Sample) ? &userData : NULL;

   // (Linux, bug 1885) After scanning devices it takes a little time for the
   // ALSA device to be available, so allow retries.
   // On my test machine, no more than 3 attempts are required.
   unsigned int maxTries = 1;
#ifdef __WXGTK__
   if (DeviceManager::Instance()->GetTimeSinceRescan() < 10)
      maxTries = 5;
#endif

   UpdateBuffers();

   unsigned long latency = GetConvertedLatencyPreference();

   for (unsigned int tries = 0; tries < maxTries; tries++) {
      mLastPaError = Pa_OpenStream( &mPortStreamV19,
                                    useCapture ? &captureParameters : NULL,
                                    usePlayback ? &playbackParameters : NULL,
                                    mRate, latency,
                                    paNoFlag,
                                    audacityAudioCallback, lpUserData );
      if (mLastPaError == paNoError) {
         break;
      }
      std::cout << "Attempt " << (1 + tries) << " to open capture stream failed with: " << mLastPaError << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }

#if (defined(__WXMAC__) || defined(__WXMSW__)) && wxCHECK_VERSION(3,1,0)
   // Don't want the system to sleep while audio I/O is active
   if (mPortStreamV19 != NULL && mLastPaError == paNoError) {
      wxPowerResource::Acquire(wxPOWER_RESOURCE_SCREEN, _("Tenacity Audio"));
   }
#endif

   return (mLastPaError == paNoError);
}

std::string AudioIO::LastPaErrorString()
{
   return std::string(std::to_string(mLastPaError) + 
                      std::string(Pa_GetErrorText(mLastPaError))
   );
}

void AudioIO::StartMonitoring( const AudioIOStartStreamOptions &options )
{
   if ( mPortStreamV19 || mStreamToken )
      return;

   bool success;
   auto captureFormat = QualitySettings::SampleFormatChoice();
   auto captureChannels = AudioIORecordChannels.Read();
   gPrefs->Read(wxT("/AudioIO/SWPlaythrough"), &mSoftwarePlaythrough, false);
   int playbackChannels = 0;

   if (mSoftwarePlaythrough)
      playbackChannels = 2;

   // FIXME: TRAP_ERR StartPortAudioStream (a PaError may be present)
   // but StartPortAudioStream function only returns true or false.
   mUsingAlsa = false;
   success = StartPortAudioStream(options, (unsigned int)playbackChannels,
                                  (unsigned int)captureChannels,
                                  captureFormat);

   auto pOwningProject = mOwningProject.lock();
   if (!success) {
      auto msg = XO("Error opening recording device.\nError code: %s")
         .Format( Get()->LastPaErrorString() );
      throw SimpleMessageBoxException(ExceptionType::Internal, msg, XO("Error"), wxT("Error_opening_sound_device"));
      return;
   }

   wxCommandEvent e(EVT_AUDIOIO_MONITOR);
   e.SetEventObject( pOwningProject.get() );
   e.SetInt(true);
   wxTheApp->ProcessEvent(e);

   // FIXME: TRAP_ERR PaErrorCode 'noted' but not reported in StartMonitoring.
   // Now start the PortAudio stream!
   // TODO: ? Factor out and reuse error reporting code from end of 
   // AudioIO::StartStream?
   mLastPaError = Pa_StartStream( mPortStreamV19 );

   // Update UI display only now, after all possibilities for error are past.
   auto pListener = GetListener();
   if ((mLastPaError == paNoError) && pListener) {
      // advertise the chosen I/O sample rate to the UI
      pListener->OnAudioIORate((int)mRate);
   }
}

int AudioIO::StartStream(const TransportTracks &tracks,
                         double t0, double t1,
                         const AudioIOStartStreamOptions &options)
{
   mLostSamples = 0;
   mLostCaptureIntervals.clear();
   mDetectDropouts =
      gPrefs->Read( WarningDialogKey(wxT("DropoutDetected")), true ) != 0;
   auto cleanup = finally ( [this] { ClearRecordingException(); } );

   if( IsBusy() )
      return 0;

   // We just want to set mStreamToken to -1 - this way avoids
   // an extremely rare but possible race condition, if two functions
   // somehow called StartStream at the same time...
   mStreamToken--;
   if (mStreamToken != -1)
      return 0;

   // TODO: we don't really need to close and reopen stream if the
   // format matches; however it's kind of tricky to keep it open...
   //
   //   if (sampleRate == mRate &&
   //       playbackChannels == mNumPlaybackChannels &&
   //       captureChannels == mNumCaptureChannels &&
   //       captureFormat == mCaptureFormat) {

   if (mPortStreamV19) {
      StopStream();
      while(mPortStreamV19)
         std::this_thread::sleep_for(std::chrono::milliseconds(50));
   }

#ifdef __WXGTK__
   // Detect whether ALSA is the chosen host, and do the various involved MIDI
   // timing compensations only then.
   mUsingAlsa = (AudioIOHost.Read() == L"ALSA");
#endif

   gPrefs->Read(wxT("/AudioIO/SWPlaythrough"), &mSoftwarePlaythrough, false);
   gPrefs->Read(wxT("/AudioIO/SoundActivatedRecord"), &mPauseRec, false);
   gPrefs->Read(wxT("/AudioIO/Microfades"), &mbMicroFades, false);
   int silenceLevelDB;
   gPrefs->Read(wxT("/AudioIO/SilenceLevel"), &silenceLevelDB, -50);
   int dBRange = DecibelScaleCutoff.Read();
   if(silenceLevelDB < -dBRange)
   {
      silenceLevelDB = -dBRange + 3;
      // meter range was made smaller than SilenceLevel
      // so set SilenceLevel reasonable

      // PRL:  update prefs, or correct it only in-session?
      // The behavior (as of 2.3.1) was the latter, the code suggested that
      // the intent was the former;  I preserve the behavior, but uncomment
      // this if you disagree.
      // gPrefs->Write(wxT("/AudioIO/SilenceLevel"), silenceLevelDB);
      // gPrefs->Flush();
   }
   mSilenceLevel = DB_TO_LINEAR(silenceLevelDB);  // meter goes -dBRange dB -> 0dB

   // Clamp pre-roll so we don't play before time 0
   const auto preRoll = std::max(0.0, std::min(t0, options.preRoll));
   mRecordingSchedule = {};
   mRecordingSchedule.mPreRoll = preRoll;
   mRecordingSchedule.mLatencyCorrection =
      AudioIOLatencyCorrection.Read() / 1000.0;
   mRecordingSchedule.mDuration = t1 - t0;
   if (options.pCrossfadeData)
      mRecordingSchedule.mCrossfadeData.swap( *options.pCrossfadeData );

   mListener = options.listener;
   mRate    = options.rate;

   mSeek    = 0;
   mLastRecordingOffset = 0;
   mCaptureTracks = tracks.captureTracks;
   mPlaybackTracks = tracks.playbackTracks;

   bool commit = false;
   auto cleanupTracks = finally([&]{
      if (!commit) {
         // Don't keep unnecessary shared pointers to tracks
         mPlaybackTracks.clear();
         mCaptureTracks.clear();
         for(auto &ext : Extensions())
            ext.AbortOtherStream();

         // Don't cause a busy wait in the audio thread after stopping scrubbing
         mPlaybackSchedule.ResetMode();
      }
   });

   mPlaybackBuffers.reset();
   mPlaybackMixers.reset();
   mCaptureBuffers.reset();
   mResample.reset();
   mTimeQueue.mData.reset();

   mPlaybackSchedule.Init(
      t0, t1, options, mCaptureTracks.empty() ? nullptr : &mRecordingSchedule );
   const bool scrubbing = mPlaybackSchedule.Interactive();

   unsigned int playbackChannels = 0;
   unsigned int captureChannels = 0;
   sampleFormat captureFormat = floatSample;

   auto pListener = GetListener();

   if (tracks.playbackTracks.size() > 0
      || tracks.otherPlayableTracks.size() > 0)
      playbackChannels = 2;

   if (mSoftwarePlaythrough)
      playbackChannels = 2;

   if (tracks.captureTracks.size() > 0)
   {
      // For capture, every input channel gets its own track
      captureChannels = mCaptureTracks.size();
      // I don't deal with the possibility of the capture tracks
      // having different sample formats, since it will never happen
      // with the current code.  This code wouldn't *break* if this
      // assumption was false, but it would be sub-optimal.  For example,
      // if the first track was 16-bit and the second track was 24-bit,
      // we would set the sound card to capture in 16 bits and the second
      // track wouldn't get the benefit of all 24 bits the card is capable
      // of.
      captureFormat = mCaptureTracks[0]->GetSampleFormat();

      // Tell project that we are about to start recording
      if (pListener)
         pListener->OnAudioIOStartRecording();
   }

   bool successAudio;

   successAudio = StartPortAudioStream(options, playbackChannels,
                                       captureChannels, captureFormat);
#ifdef EXPERIMENTAL_MIDI_OUT
   auto range = Extensions();
   successAudio = successAudio &&
      std::all_of(range.begin(), range.end(),
         [this, &tracks, t0](auto &ext){
            return ext.StartOtherStream( tracks,
              (mPortStreamV19 != NULL && mLastPaError == paNoError)
                 ? Pa_GetStreamInfo(mPortStreamV19) : nullptr,
              t0, mRate ); });
#endif

   if (!successAudio) {
      if (pListener && captureChannels > 0)
         pListener->OnAudioIOStopRecording();
      mStreamToken = 0;

      return 0;
   }

   if ( ! AllocateBuffers( options, tracks, t0, t1, options.rate, scrubbing ) )
      return 0;

   if (mNumPlaybackChannels > 0)
   {
      if (auto pOwningProject = mOwningProject.lock()) {
         auto & em = RealtimeEffectManager::Get(*pOwningProject);
         // Setup for realtime playback at the rate of the realtime
         // stream, not the rate of the track.
         em.RealtimeInitialize(mRate);

         // The following adds a NEW effect processor for each logical track and the
         // group determination should mimic what is done in audacityAudioCallback()
         // when calling RealtimeProcess().
         int group = 0;
         for (size_t i = 0, cnt = mPlaybackTracks.size(); i < cnt;)
         {
            const WaveTrack *vt = mPlaybackTracks[i].get();

            // TODO: more-than-two-channels
            unsigned chanCnt = TrackList::Channels(vt).size();
            i += chanCnt;

            // Setup for realtime playback at the rate of the realtime
            // stream, not the rate of the track.
            em.RealtimeAddProcessor(group++, std::min(2u, chanCnt), mRate);
         }
      }
   }

   if (options.pStartTime)
   {
      // Calculate the NEW time position
      const auto time = mPlaybackSchedule.ClampTrackTime( *options.pStartTime );

      // Main thread's initialization of mTime
      mPlaybackSchedule.SetTrackTime( time );

      // Reset mixer positions for all playback tracks
      unsigned numMixers = mPlaybackTracks.size();
      for (unsigned ii = 0; ii < numMixers; ++ii)
         mPlaybackMixers[ii]->Reposition( time );
      mPlaybackSchedule.RealTimeInit( time );
   }
   
   // Now that we are done with SetTrackTime():
   mTimeQueue.mLastTime = mPlaybackSchedule.GetTrackTime();
   if (mTimeQueue.mData)
      mTimeQueue.mData[0] = mTimeQueue.mLastTime;
   // else recording only without overdub

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   if (scrubbing)
   {
      const auto &scrubOptions = *options.pScrubbingOptions;
      mScrubState =
         std::make_unique<ScrubState>(
            mPlaybackSchedule.mT0,
            mRate,
            scrubOptions);
      mScrubDuration = 0;
      mSilentScrub = false;
   }
   else
      mScrubState.reset();
#endif

   // We signal the audio thread to call TrackBufferExchange, to prime the RingBuffers
   // so that they will have data in them when the stream starts.  Having the
   // audio thread call TrackBufferExchange here makes the code more predictable, since
   // TrackBufferExchange will ALWAYS get called from the Audio thread.
   mAudioThreadShouldCallTrackBufferExchangeOnce = true;

   while( mAudioThreadShouldCallTrackBufferExchangeOnce ) {
      auto interval = 50ull;
      if (options.playbackStreamPrimer) {
         interval = options.playbackStreamPrimer();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
   }

   if(mNumPlaybackChannels > 0 || mNumCaptureChannels > 0) {

#ifdef REALTIME_ALSA_THREAD
      // PRL: Do this in hope of less thread scheduling jitter in calls to
      // audacityAudioCallback.
      // Not needed to make audio playback work smoothly.
      // But needed in case we also play MIDI, so that the variable "offset"
      // in AudioIO::MidiTime() is a better approximation of the duration
      // between the call of audacityAudioCallback and the actual output of
      // the first audio sample.
      // (Which we should be able to determine from fields of
      // PaStreamCallbackTimeInfo, but that seems not to work as documented with
      // ALSA.)
      if (mUsingAlsa)
         // Perhaps we should do this only if also playing MIDI ?
         PaAlsa_EnableRealtimeScheduling( mPortStreamV19, 1 );
#endif

      //
      // Generate a unique value each time, to be returned to
      // clients accessing the AudioIO API, so they can query if they
      // are the ones who have reserved AudioIO or not.
      //
      // It is important to set this before setting the portaudio stream in
      // motion -- otherwise it may play an unspecified number of leading
      // zeroes.
      mStreamToken = (++mNextStreamToken);

      // This affects the AudioThread (not the portaudio callback).
      // Probably not needed so urgently before portaudio thread start for usual
      // playback, since our ring buffers have been primed already with 4 sec
      // of audio, but then we might be scrubbing, so do it.
      mAudioThreadTrackBufferExchangeLoopRunning = true;
      mForceFadeOut.store(false, std::memory_order_relaxed);

      // Now start the PortAudio stream!
      PaError err;
      err = Pa_StartStream( mPortStreamV19 );

      if( err != paNoError )
      {
         mStreamToken = 0;
         mAudioThreadTrackBufferExchangeLoopRunning = false;
         if (pListener && mNumCaptureChannels > 0)
            pListener->OnAudioIOStopRecording();
         StartStreamCleanup();
         // PRL: PortAudio error messages are sadly not internationalized
         throw SimpleMessageBoxException(ExceptionType::Internal, StringLiteral(Pa_GetErrorText(err)), StringLiteral("PortAudio Stream Error"));
         return 0;
      }
   }

   // Update UI display only now, after all possibilities for error are past.
   if (pListener) {
      // advertise the chosen I/O sample rate to the UI
      pListener->OnAudioIORate((int)mRate);
   }

   auto pOwningProject = mOwningProject.lock();
   if (mNumPlaybackChannels > 0)
   {
      wxCommandEvent e(EVT_AUDIOIO_PLAYBACK);
      e.SetEventObject( pOwningProject.get() );
      e.SetInt(true);
      wxTheApp->ProcessEvent(e);
   }

   if (mNumCaptureChannels > 0)
   {
      wxCommandEvent e(EVT_AUDIOIO_CAPTURE);
      e.SetEventObject( pOwningProject.get() );
      e.SetInt(true);
      wxTheApp->ProcessEvent(e);
   }

   commit = true;
   return mStreamToken;
}

void AudioIO::DelayActions(bool recording)
{
   mDelayingActions = recording;
}

bool AudioIO::DelayingActions() const
{
   return mDelayingActions || (mPortStreamV19 && mNumCaptureChannels > 0);
}

void AudioIO::CallAfterRecording(PostRecordingAction action)
{
   if (!action)
      return;

   {
      std::lock_guard<std::mutex> guard{ mPostRecordingActionMutex };
      if (mPostRecordingAction) {
         // Enqueue it, even if perhaps not still recording,
         // but it wasn't cleared yet
         mPostRecordingAction = [
            prevAction = std::move(mPostRecordingAction),
            nextAction = std::move(action)
         ]{ prevAction(); nextAction(); };
         return;
      }
      else if (DelayingActions()) {
         mPostRecordingAction = std::move(action);
         return;
      }
   }

   // Don't delay it except until idle time.
   // (Recording might start between now and then, but won't go far before
   // the action is done.  So the system isn't bulletproof yet.)
   wxTheApp->CallAfter(std::move(action));
}

bool AudioIO::AllocateBuffers(
   const AudioIOStartStreamOptions &options,
   const TransportTracks &tracks, double t0, double t1, double sampleRate,
   bool scrubbing )
{
   bool success = false;
   auto cleanup = finally([&]{
      if (!success) StartStreamCleanup( false );
   });

   //
   // The (audio) stream has been opened successfully (assuming we tried
   // to open it). We now proceed to
   // allocate the memory structures the stream will need.
   //

   //
   // The RingBuffer sizes, and the max amount of the buffer to
   // fill at a time, both grow linearly with the number of
   // tracks.  This allows us to scale up to many tracks without
   // killing performance.
   //

   // real playback time to produce with each filling of the buffers
   // by the Audio thread (except at the end of playback):
   // usually, make fillings fewer and longer for less CPU usage.
   // But for useful scrubbing, we can't run too far ahead without checking
   // mouse input, so make fillings more and shorter.
   // What Audio thread produces for playback is then consumed by the PortAudio
   // thread, in many smaller pieces.
   double playbackTime = 4.0;
   if (scrubbing)
      // Specify a very short minimum batch for non-seek scrubbing, to allow
      // more frequent polling of the mouse
      playbackTime =
         lrint(options.pScrubbingOptions->delay * mRate) / mRate;
   
   assert( playbackTime >= 0 );
   mPlaybackSamplesToCopy = playbackTime * mRate;

   // Capacity of the playback buffer.
   mPlaybackRingBufferSecs = 10.0;

   mCaptureRingBufferSecs =
      4.5 + 0.5 * std::min(size_t(16), mCaptureTracks.size());
   mMinCaptureSecsToCopy =
      0.2 + 0.2 * std::min(size_t(16), mCaptureTracks.size());

   mTimeQueue.mHead = {};
   mTimeQueue.mTail = {};
   bool bDone;
   do
   {
      bDone = true; // assume success
      try
      {
         if( mNumPlaybackChannels > 0 ) {
            // Allocate output buffers.  For every output track we allocate
            // a ring buffer of ten seconds
            auto playbackBufferSize =
               (size_t)lrint(mRate * mPlaybackRingBufferSecs);

            mPlaybackBuffers.reinit(mPlaybackTracks.size());
            mPlaybackMixers.reinit(mPlaybackTracks.size());

            const Mixer::WarpOptions &warpOptions =
#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
               scrubbing
                  ? Mixer::WarpOptions
                     (ScrubbingOptions::MinAllowedScrubSpeed(),
                      ScrubbingOptions::MaxAllowedScrubSpeed())
                  :
#endif
                    Mixer::WarpOptions(mPlaybackSchedule.mEnvelope);

            mPlaybackQueueMinimum = mPlaybackSamplesToCopy;
            if (scrubbing)
               // Specify enough playback RingBuffer latency so we can refill
               // once every seek stutter without falling behind the demand.
               // (Scrub might switch in and out of seeking with left mouse
               // presses in the ruler)
               mPlaybackQueueMinimum = lrint(
                  2 * options.pScrubbingOptions->minStutterTime * mRate );
            mPlaybackQueueMinimum =
               std::min( mPlaybackQueueMinimum, playbackBufferSize );

            for (unsigned int i = 0; i < mPlaybackTracks.size(); i++)
            {
               // Bug 1763 - We must fade in from zero to avoid a click on starting.
               mPlaybackTracks[i]->SetOldChannelGain(0, 0.0);
               mPlaybackTracks[i]->SetOldChannelGain(1, 0.0);

               mPlaybackBuffers[i] =
                  std::make_unique<RingBuffer>(floatSample, playbackBufferSize);
               const auto timeQueueSize = 1 +
                  (playbackBufferSize + TimeQueueGrainSize - 1)
                     / TimeQueueGrainSize;
               mTimeQueue.mData.reinit( timeQueueSize );
               mTimeQueue.mSize = timeQueueSize;

               // use track time for the end time, not real time!
               WaveTrackConstArray mixTracks;
               mixTracks.push_back(mPlaybackTracks[i]);

               double endTime;
               if (make_iterator_range(tracks.prerollTracks)
                      .contains(mPlaybackTracks[i]))
                  // Stop playing this track after pre-roll
                  endTime = t0;
               else
                  // Pass t1 -- not mT1 as may have been adjusted for latency
                  // -- so that overdub recording stops playing back samples
                  // at the right time, though transport may continue to record
                  endTime = t1;

               mPlaybackMixers[i] = std::make_unique<Mixer>
                  (mixTracks,
                  // Don't throw for read errors, just play silence:
                  false,
                  warpOptions,
                  mPlaybackSchedule.mT0,
                  endTime,
                  1,
                  std::max( mPlaybackSamplesToCopy, mPlaybackQueueMinimum ),
                  false,
                  mRate, floatSample,
                  false, // low quality dithering and resampling
                  nullptr,
                  false // don't apply track gains
               );
            }
         }

         if( mNumCaptureChannels > 0 )
         {
            // Allocate input buffers.  For every input track we allocate
            // a ring buffer of five seconds
            auto captureBufferSize =
               (size_t)(mRate * mCaptureRingBufferSecs + 0.5);

            // In the extraordinarily rare case that we can't even afford
            // 100 samples, just give up.
            if(captureBufferSize < 100)
            {
               throw std::bad_alloc();
            }

            mCaptureBuffers.reinit(mCaptureTracks.size());
            mResample.reinit(mCaptureTracks.size());
            mFactor = sampleRate / mRate;

            for( unsigned int i = 0; i < mCaptureTracks.size(); i++ )
            {
               mCaptureBuffers[i] = std::make_unique<RingBuffer>(
                  mCaptureTracks[i]->GetSampleFormat(), captureBufferSize );
               mResample[i] =
                  std::make_unique<Resample>(true, mFactor, mFactor);
                  // constant rate resampling
            }
         }
      }
      catch(std::bad_alloc&)
      {
         // Oops!  Ran out of memory.  This is pretty rare, so we'll just
         // try deleting everything, halving our buffer size, and try again.
         StartStreamCleanup(true);
         mPlaybackRingBufferSecs *= 0.5;
         mPlaybackSamplesToCopy /= 2;
         mCaptureRingBufferSecs *= 0.5;
         mMinCaptureSecsToCopy *= 0.5;
         bDone = false;

         // In the extraordinarily rare case that we can't even afford 100
         // samples, just give up.
         auto playbackBufferSize =
            (size_t)lrint(mRate * mPlaybackRingBufferSecs);
         if(playbackBufferSize < 100 || mPlaybackSamplesToCopy < 100)
         {
            throw std::bad_alloc();
         }
      }
   } while(!bDone);
   
   success = true;
   return true;
}

void AudioIO::StartStreamCleanup(bool bOnlyBuffers)
{
   if (mNumPlaybackChannels > 0)
   {
      if (auto pOwningProject = mOwningProject.lock())
         RealtimeEffectManager::Get(*pOwningProject).RealtimeFinalize();
   }

   mPlaybackBuffers.reset();
   mPlaybackMixers.reset();
   mCaptureBuffers.reset();
   mResample.reset();
   mTimeQueue.mData.reset();

   if(!bOnlyBuffers)
   {
      Pa_AbortStream( mPortStreamV19 );
      Pa_CloseStream( mPortStreamV19 );
      mPortStreamV19 = NULL;
      mStreamToken = 0;
   }

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   mScrubState.reset();
#endif
}

bool AudioIO::IsAvailable(TenacityProject &project) const
{
   auto pOwningProject = mOwningProject.lock();
   return !pOwningProject || pOwningProject.get() == &project;
}

void AudioIO::SetMeters()
{
   if (auto pInputMeter = mInputMeter.lock())
      pInputMeter->Reset(mRate, true);
   if (auto pOutputMeter = mOutputMeter.lock())
      pOutputMeter->Reset(mRate, true);

   mUpdateMeters = true;
}

void AudioIO::StopStream()
{
   auto cleanup = finally ( [this] {
      ClearRecordingException();
      mRecordingSchedule.mCrossfadeData.clear(); // free arrays
   } );

   if( mPortStreamV19 == NULL )
      return;

   if (Pa_IsStreamStopped( mPortStreamV19 ))
   {
      return;
   }

#if (defined(__WXMAC__) || defined(__WXMSW__)) && wxCHECK_VERSION(3,1,0)
   // Re-enable system sleep
   wxPowerResource::Release(wxPOWER_RESOURCE_SCREEN);
#endif
 
   if( mAudioThreadTrackBufferExchangeLoopRunning )
   {
      // PortAudio callback can use the information that we are stopping to fade
      // out the audio.  Give PortAudio callback a chance to do so.
      mForceFadeOut.store(true, std::memory_order_relaxed);
      auto latency = static_cast<long>(AudioIOLatencyDuration.Read());
      // If we can gracefully fade out in 200ms, with the faded-out play buffers making it through
      // the sound card, then do so.  If we can't, don't wait around.  Just stop quickly and accept
      // there will be a click.
      if( mbMicroFades  && (latency < 150 ))
         std::this_thread::sleep_for(std::chrono::milliseconds(latency + 50));
   }

   std::lock_guard<std::mutex> locker(mSuspendAudioThread);

   // No longer need effects processing
   if (mNumPlaybackChannels > 0)
   {
      if (auto pOwningProject = mOwningProject.lock())
         RealtimeEffectManager::Get(*pOwningProject).RealtimeFinalize();
   }

   //
   // We got here in one of two ways:
   //
   // 1. The user clicked the stop button and we therefore want to stop
   //    as quickly as possible.  So we use AbortStream().  If this is
   //    the case the portaudio stream is still in the Running state
   //    (see PortAudio state machine docs).
   //
   // 2. The callback told PortAudio to stop the stream since it had
   //    reached the end of the selection.  The UI thread discovered
   //    this by noticing that AudioIO::IsActive() returned false.
   //    IsActive() (which calls Pa_GetStreamActive()) will not return
   //    false until all buffers have finished playing, so we can call
   //    AbortStream without losing any samples.  If this is the case
   //    we are in the "callback finished state" (see PortAudio state
   //    machine docs).
   //
   // The moral of the story: We can call AbortStream safely, without
   // losing samples.
   //
   // DMM: This doesn't seem to be true; it seems to be necessary to
   // call StopStream if the callback brought us here, and AbortStream
   // if the user brought us here.
   //

   mAudioThreadTrackBufferExchangeLoopRunning = false;

   // Audacity can deadlock if it tries to update meters while
   // we're stopping PortAudio (because the meter updating code
   // tries to grab a UI mutex while PortAudio tries to join a
   // pthread).  So we tell the callback to stop updating meters,
   // and wait until the callback has left this part of the code
   // if it was already there.
   mUpdateMeters = false;
   while(mUpdatingMeters) {
      ::wxSafeYield();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
   }

   if (mPortStreamV19) {
      Pa_AbortStream( mPortStreamV19 );
      Pa_CloseStream( mPortStreamV19 );
      mPortStreamV19 = NULL;
   }

   for( auto &ext : Extensions() )
      ext.StopOtherStream();

   auto pListener = GetListener();
   
   // If there's no token, we were just monitoring, so we can
   // skip this next part...
   if (mStreamToken > 0) {
      // In either of the above cases, we want to make sure that any
      // capture data that made it into the PortAudio callback makes it
      // to the target WaveTrack.  To do this, we ask the audio thread to
      // call TrackBufferExchange one last time (it normally would not do so since
      // Pa_GetStreamActive() would now return false
      mAudioThreadShouldCallTrackBufferExchangeOnce = true;

      while( mAudioThreadShouldCallTrackBufferExchangeOnce )
      {
         //FIXME: Seems like this block of the UI thread isn't bounded,
         //but we cannot allow event handlers to see incompletely terminated
         //AudioIO state with wxYield (or similar functions)
         std::this_thread::sleep_for(std::chrono::milliseconds( 50 ));
      }

      //
      // Everything is taken care of.  Now, just free all the resources
      // we allocated in StartStream()
      //

      if (mPlaybackTracks.size() > 0)
      {
         mPlaybackBuffers.reset();
         mPlaybackMixers.reset();
         mTimeQueue.mData.reset();
      }

      //
      // Offset all recorded tracks to account for latency
      //
      if (mCaptureTracks.size() > 0)
      {
         mCaptureBuffers.reset();
         mResample.reset();

         //
         // We only apply latency correction when we actually played back
         // tracks during the recording. If we did not play back tracks,
         // there's nothing we could be out of sync with. This also covers the
         // case that we do not apply latency correction when recording the
         // first track in a project.
         //

         for (unsigned int i = 0; i < mCaptureTracks.size(); i++) {
            // The calls to Flush
            // may cause exceptions because of exhaustion of disk space.
            // Stop those exceptions here, or else they propagate through too
            // many parts of Audacity that are not effects or editing
            // operations.  GuardedCall ensures that the user sees a warning.

            // Also be sure to Flush each track, at the top of the guarded call,
            // relying on the guarantee that the track will be left in a flushed
            // state, though the append buffer may be lost.

            GuardedCall( [&] {
               WaveTrack* track = mCaptureTracks[i].get();

               // use No-fail-guarantee that track is flushed,
               // Partial-guarantee that some initial length of the recording
               // is saved.
               // See comments in TrackBufferExchange().
               track->Flush();
            } );
         }

         
         if (!mLostCaptureIntervals.empty())
         {
            // This scope may combine many splittings of wave tracks
            // into one transaction, lessening the number of checkpoints
            std::optional<TransactionScope> pScope;
            auto pOwningProject = mOwningProject.lock();
            if (pOwningProject) {
               auto &pIO = ProjectFileIO::Get(*pOwningProject);
               pScope.emplace(pIO.GetConnection(), "Dropouts");
            }
            for (auto &interval : mLostCaptureIntervals) {
               auto &start = interval.first;
               auto duration = interval.second;
               for (auto &track : mCaptureTracks) {
                  GuardedCall([&] {
                     track->SyncLockAdjust(start, start + duration);
                  });
               }
            }
            if (pScope)
               pScope->Commit();
         }

         if (pListener)
            pListener->OnCommitRecording();
      }
   }

   if (auto pInputMeter = mInputMeter.lock())
      pInputMeter->Reset(mRate, false);

   if (auto pOutputMeter = mOutputMeter.lock())
      pOutputMeter->Reset(mRate, false);

   mInputMeter.reset();
   mOutputMeter.reset();
   mOwningProject.reset();

   if (pListener && mNumCaptureChannels > 0)
      pListener->OnAudioIOStopRecording();

   wxTheApp->CallAfter([this]{
      if (mPortStreamV19 && mNumCaptureChannels > 0)
         // Recording was restarted between StopStream and idle time
         // So the actions can keep waiting
         return;
      // In case some other thread was waiting on the mutex too:
      std::this_thread::yield();
      std::lock_guard<std::mutex> guard{ mPostRecordingActionMutex };
      if (mPostRecordingAction) {
         mPostRecordingAction();
         mPostRecordingAction = {};
      }
      DelayActions(false);
   });

   //
   // Only set token to 0 after we're totally finished with everything
   //
   bool wasMonitoring = mStreamToken == 0;
   mStreamToken = 0;

   if (mNumPlaybackChannels > 0)
   {
      wxCommandEvent e(EVT_AUDIOIO_PLAYBACK);
      auto pOwningProject = mOwningProject.lock();
      e.SetEventObject(pOwningProject.get());
      e.SetInt(false);
      wxTheApp->ProcessEvent(e);
   }
   
   if (mNumCaptureChannels > 0)
   {
      wxCommandEvent e(wasMonitoring ? EVT_AUDIOIO_MONITOR : EVT_AUDIOIO_CAPTURE);
      auto pOwningProject = mOwningProject.lock();
      e.SetEventObject(pOwningProject.get());
      e.SetInt(false);
      wxTheApp->ProcessEvent(e);
   }

   mNumCaptureChannels = 0;
   mNumPlaybackChannels = 0;

   mPlaybackTracks.clear();
   mCaptureTracks.clear();

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   mScrubState.reset();
#endif

   if (pListener) {
      // Tell UI to hide sample rate
      pListener->OnAudioIORate(0);
   }

   // Don't cause a busy wait in the audio thread after stopping scrubbing
   mPlaybackSchedule.ResetMode();
}

void AudioIO::SetPaused(bool state)
{
   if (state != mPaused)
   {
      if (auto pOwningProject = mOwningProject.lock()) {
         auto &em = RealtimeEffectManager::Get(*pOwningProject);
         if (state)
            em.RealtimeSuspend();
         else
            em.RealtimeResume();
      }
   }

   mPaused = state;
}

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
void AudioIO::UpdateScrub
   (double endTimeOrSpeed, const ScrubbingOptions &options)
{
   if (mScrubState)
      mScrubState->Update(endTimeOrSpeed, options);
}

void AudioIO::StopScrub()
{
   if (mScrubState)
      mScrubState->Stop();
}

#if 0
// Only for DRAG_SCRUB
double AudioIO::GetLastScrubTime() const
{
   if (mScrubState)
      return mScrubState->LastTrackTime();
   else
      return -1.0;
}
#endif

#endif

double AudioIO::GetBestRate(bool capturing, bool playing, double sampleRate)
{
   // Check if we can use the cached value
   if (mCachedBestRateIn != 0.0 && mCachedBestRateIn == sampleRate
      && mCachedBestRatePlaying == playing && mCachedBestRateCapturing == capturing) {
      return mCachedBestRateOut;
   }

   // In order to cache the value, all early returns should instead set retval
   // and jump to finished
   double retval;

   std::vector<long> rates;
   if (capturing) std::cout << "AudioIO::GetBestRate() for capture" << std::endl;
   if (playing) std::cout << "AudioIO::GetBestRate() for playback" << std::endl;
   std::cout << "GetBestRate() suggested rate " << std::round(sampleRate) << " Hz" << std::endl;

   if (capturing && !playing) {
      rates = GetSupportedCaptureRates(-1, sampleRate);
   }
   else if (playing && !capturing) {
      rates = GetSupportedPlaybackRates(-1, sampleRate);
   }
   else {   // we assume capturing and playing - the alternative would be a
            // bit odd
      rates = GetSupportedSampleRates(-1, -1, sampleRate);
   }
   /* rem rates is the array of hardware-supported sample rates (in the current
    * configuration), sampleRate is the Project Rate (desired sample rate) */
   long rate = (long)sampleRate;

   if (make_iterator_range(rates).contains(rate)) {
      std::cout << "GetBestRate() Returning " << std::round(rate) << " Hz" << std::endl;
      retval = rate;
      goto finished;
      /* the easy case - the suggested rate (project rate) is in the list, and
       * we can just accept that and send back to the caller. This should be
       * the case for most users most of the time (all of the time on
       * Win MME as the OS does resampling) */
   }

   /* if we get here, there is a problem - the project rate isn't supported
    * on our hardware, so we can't us it. Need to come up with an alternative
    * rate to use. The process goes like this:
    * * If there are no rates to pick from, we're stuck and return 0 (error)
    * * If there are some rates, we pick the next one higher than the requested
    *   rate to use.
    * * If there aren't any higher, we use the highest available rate */

   if (rates.empty()) {
      /* we're stuck - there are no supported rates with this hardware. Error */
      std::cout << "GetBestRate() Error - no supported sample rates" << std::endl;
      retval = 0.0;
      goto finished;
   }
   int i;
   for (i = 0; i < (int)rates.size(); i++)  // for each supported rate
         {
         if (rates[i] > rate) {
            // supported rate is greater than requested rate
            std::cout << "GetBestRate() Returning next higher rate - " << std::round(rates[i]) << " Hz" << std::endl;
            retval = rates[i];
            goto finished;
         }
         }

   std::cout << "GetBestRate() Returning highest rate - " << rates.back() << " Hz" << std::endl;
   retval = rates.back(); // the highest available rate
   goto finished;

finished:
   mCachedBestRateIn = sampleRate;
   mCachedBestRateOut = retval;
   mCachedBestRatePlaying = playing;
   mCachedBestRateCapturing = capturing;
   return retval;
}

double AudioIO::GetStreamTime()
{
   // Track time readout for the main thread

   if( !IsStreamActive() )
      return BAD_STREAM_TIME;

   return mPlaybackSchedule.NormalizeTrackTime();
}

size_t AudioIO::GetCommonlyFreePlayback()
{
   auto commonlyAvail = mPlaybackBuffers[0]->AvailForPut();
   for (unsigned i = 1; i < mPlaybackTracks.size(); ++i)
      commonlyAvail = std::min(commonlyAvail,
         mPlaybackBuffers[i]->AvailForPut());
   // MB: subtract a few samples because the code in TrackBufferExchange has rounding
   // errors
   return commonlyAvail - std::min(size_t(10), commonlyAvail);
}

size_t AudioIoCallback::GetCommonlyReadyPlayback()
{
   if (mPlaybackTracks.empty())
      return 0;

   auto commonlyAvail = mPlaybackBuffers[0]->AvailForGet();
   for (unsigned i = 1; i < mPlaybackTracks.size(); ++i)
      commonlyAvail = std::min(commonlyAvail,
         mPlaybackBuffers[i]->AvailForGet());
   return commonlyAvail;
}

size_t AudioIO::GetCommonlyAvailCapture()
{
   auto commonlyAvail = mCaptureBuffers[0]->AvailForGet();
   for (unsigned i = 1; i < mCaptureTracks.size(); ++i)
      commonlyAvail = std::min(commonlyAvail,
         mCaptureBuffers[i]->AvailForGet());
   return commonlyAvail;
}

// This method is the data gateway between the audio thread (which
// communicates with the disk) and the PortAudio callback thread
// (which communicates with the audio device).
void AudioIO::TrackBufferExchange()
{
   FillPlayBuffers();
   DrainRecordBuffers();
}

void AudioIO::FillPlayBuffers()
{
   if (mPlaybackTracks.empty())
      return;

   // Though extremely unlikely, it is possible that some buffers
   // will have more samples available than others.  This could happen
   // if we hit this code during the PortAudio callback.  To keep
   // things simple, we only write as much data as is vacant in
   // ALL buffers, and advance the global time by that much.
   auto nAvailable = GetCommonlyFreePlayback();

   // Don't fill the buffers at all unless we can do the
   // full mMaxPlaybackSecsToCopy.  This improves performance
   // by not always trying to process tiny chunks, eating the
   // CPU unnecessarily.
   if (nAvailable < mPlaybackSamplesToCopy)
      return;

   // More than mPlaybackSamplesToCopy might be copied:
   // May produce a larger amount when initially priming the buffer, or
   // perhaps again later in play to avoid underfilling the queue and falling
   // behind the real-time demand on the consumer side in the callback.
   auto nReady = GetCommonlyReadyPlayback();
   auto nNeeded =
      mPlaybackQueueMinimum - std::min(mPlaybackQueueMinimum, nReady);

   // wxASSERT( nNeeded <= nAvailable );

   // Limit maximum buffer size (increases performance)
   auto available = std::min( nAvailable,
      std::max( nNeeded, mPlaybackSamplesToCopy ) );

   // msmeyer: When playing a very short selection in looped
   // mode, the selection must be copied to the buffer multiple
   // times, to ensure, that the buffer has a reasonable size
   // This is the purpose of this loop.
   // PRL: or, when scrubbing, we may get work repeatedly from the
   // user interface.
   bool done = false;
   do {
      const auto [frames, toProduce, progress] = GetPlaybackSlice(available);

      // Update the time queue.  This must be done before writing to the
      // ring buffers of samples, for proper synchronization with the
      // consumer side in the PortAudio thread, which reads the time
      // queue after reading the sample queues.  The sample queues use
      // atomic variables, the time queue doesn't.
      mTimeQueue.Producer( mPlaybackSchedule, mRate,
         (mPlaybackSchedule.Interactive() ? mScrubSpeed : 1.0),
         frames);

      for (size_t i = 0; i < mPlaybackTracks.size(); i++)
      {
         // The mixer here isn't actually mixing: it's just doing
         // resampling, format conversion, and possibly time track
         // warping
         if (frames > 0)
         {
            size_t produced = 0;
            if ( toProduce )
               produced = mPlaybackMixers[i]->Process( toProduce );
            //wxASSERT(processed <= toProduce);
            auto warpedSamples = mPlaybackMixers[i]->GetBuffer();
            const auto put = mPlaybackBuffers[i]->Put(
               warpedSamples, floatSample, produced, frames - produced);
            // wxASSERT(put == frames);
            // but we can't assert in this thread
            wxUnusedVar(put);
         }
      }

      available -= frames;
      wxASSERT(available >= 0);

      done = RepositionPlayback(frames, available, progress);
   } while (!done);
}

PlaybackSlice AudioIO::GetPlaybackSlice(const size_t available)
{
   // How many samples to produce for each channel.
   auto frames = available;
   bool progress = true;
   auto toProduce = frames;
#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   if (mPlaybackSchedule.Interactive())
      // scrubbing and play-at-speed are not limited by the real time
      // and length accumulators
      toProduce =
      frames = limitSampleBufferSize(frames, mScrubDuration);
   else
#endif
   {
      double deltat = frames / mRate;
      const auto realTimeRemaining = mPlaybackSchedule.RealTimeRemaining();
      if (deltat > realTimeRemaining)
      {
         frames = realTimeRemaining * mRate;
         toProduce = frames;

         // Don't fall into an infinite loop, if loop-playing a selection
         // that is so short, it has no samples: detect that case
         progress =
            !(mPlaybackSchedule.Looping() &&
              mPlaybackSchedule.mWarpedTime == 0.0 && frames == 0);
         mPlaybackSchedule.RealTimeAdvance( realTimeRemaining );
      }
      else
         mPlaybackSchedule.RealTimeAdvance( deltat );
   }

   if (!progress)
      frames = available, toProduce = 0;
#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   else if ( mPlaybackSchedule.Interactive() && mSilentScrub)
      toProduce = 0;
#endif

   return { available, frames, toProduce, progress };
}

bool AudioIO::RepositionPlayback(size_t frames, size_t available, bool progress)
{
   bool done = false;
   switch (mPlaybackSchedule.mPlayMode)
   {
   #ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   case PlaybackSchedule::PLAY_SCRUB:
   case PlaybackSchedule::PLAY_AT_SPEED:
   case PlaybackSchedule::PLAY_KEYBOARD_SCRUB:
   {
      mScrubDuration -= frames;
      wxASSERT(mScrubDuration >= 0);
      done = (available == 0);
      if (!done && mScrubDuration <= 0)
      {
         sampleCount startSample, endSample;
         mScrubState->Get(
            startSample, endSample, available, mScrubDuration);
         if (mScrubDuration < 0)
         {
            // Can't play anything
            // Stop even if we don't fill up available
            mScrubDuration = 0;
            done = true;
         }
         else
         {
            mSilentScrub = (endSample == startSample);
            double startTime, endTime;
            startTime = startSample.as_double() / mRate;
            endTime = endSample.as_double() / mRate;
            auto diff = (endSample - startSample).as_long_long();
            if (mScrubDuration == 0)
               mScrubSpeed = 0;
            else
               mScrubSpeed =
                  double(diff) / mScrubDuration.as_double();
            if (!mSilentScrub)
            {
               for (size_t i = 0; i < mPlaybackTracks.size(); i++) {
                  if (mPlaybackSchedule.mPlayMode == PlaybackSchedule::PLAY_AT_SPEED)
                     mPlaybackMixers[i]->SetSpeedForPlayAtSpeed(mScrubSpeed);
                  else if (mPlaybackSchedule.mPlayMode == PlaybackSchedule::PLAY_KEYBOARD_SCRUB)
                     mPlaybackMixers[i]->SetSpeedForKeyboardScrubbing(mScrubSpeed, startTime);
                  else
                     mPlaybackMixers[i]->SetTimesAndSpeed(
                        startTime, endTime, fabs( mScrubSpeed ));
               }
            }
            mTimeQueue.mLastTime = startTime;
         }
      }
   }
      break;
   #endif
   case PlaybackSchedule::PLAY_LOOPED:
   {
      done = !progress || (available == 0);
      // msmeyer: If playing looped, check if we are at the end of the buffer
      // and if yes, restart from the beginning.
      if (mPlaybackSchedule.RealTimeRemaining() <= 0)
      {
         for (size_t i = 0; i < mPlaybackTracks.size(); i++)
            mPlaybackMixers[i]->Restart();
         mPlaybackSchedule.RealTimeRestart();
      }
   }
      break;
   default:
      done = true;
      break;
   }
   return done;
}

void AudioIO::DrainRecordBuffers()
{
   if (mRecordingException || mCaptureTracks.empty())
      return;

   auto delayedHandler = [this] ( TenacityException * pException ) {
      // In the main thread, stop recording
      // This is one place where the application handles disk
      // exhaustion exceptions from wave track operations, without rolling
      // back to the last pushed undo state.  Instead, partial recording
      // results are pushed as a NEW undo state.  For this reason, as
      // commented elsewhere, we want an exception safety guarantee for
      // the output wave tracks, after the failed append operation, that
      // the tracks remain as they were after the previous successful
      // (block-level) appends.

      // Note that the Flush in StopStream() may throw another exception,
      // but StopStream() contains that exception, and the logic in
      // TenacityException::DelayedHandlerAction prevents redundant message
      // boxes.
      StopStream();
      DefaultDelayedHandlerAction( pException );
   };

   GuardedCall( [&] {
      // start record buffering
      const auto avail = GetCommonlyAvailCapture(); // samples
      const auto remainingTime =
         std::max(0.0, mRecordingSchedule.ToConsume());
      // This may be a very big double number:
      const auto remainingSamples = remainingTime * mRate;
      bool latencyCorrected = true;

      double deltat = avail / mRate;

      if (mAudioThreadShouldCallTrackBufferExchangeOnce ||
          deltat >= mMinCaptureSecsToCopy)
      {
         // This scope may combine many appendings of wave tracks,
         // and also an autosave, into one transaction,
         // lessening the number of checkpoints
         std::optional<TransactionScope> pScope;
         auto pOwningProject = mOwningProject.lock();
         if (pOwningProject) {
            auto &pIO = ProjectFileIO::Get(*pOwningProject);
            pScope.emplace(pIO.GetConnection(), "Recording");
         }

         bool newBlocks = false;

         // Append captured samples to the end of the WaveTracks.
         // The WaveTracks have their own buffering for efficiency.
         auto numChannels = mCaptureTracks.size();

         for( size_t i = 0; i < numChannels; i++ )
         {
            sampleFormat trackFormat = mCaptureTracks[i]->GetSampleFormat();

            size_t discarded = 0;

            if (!mRecordingSchedule.mLatencyCorrected) {
               const auto correction = mRecordingSchedule.TotalCorrection();
               if (correction >= 0) {
                  // Rightward shift
                  // Once only (per track per recording), insert some initial
                  // silence.
                  size_t size = floor( correction * mRate * mFactor);
                  SampleBuffer temp(size, trackFormat);
                  ClearSamples(temp.ptr(), trackFormat, 0, size);
                  mCaptureTracks[i]->Append(temp.ptr(), trackFormat, size, 1);
               }
               else {
                  // Leftward shift
                  // discard some samples from the ring buffers.
                  size_t size = floor(
                     mRecordingSchedule.ToDiscard() * mRate );

                  // The ring buffer might have grown concurrently -- don't discard more
                  // than the "avail" value noted above.
                  discarded = mCaptureBuffers[i]->Discard(std::min(avail, size));

                  if (discarded < size)
                     // We need to visit this again to complete the
                     // discarding.
                     latencyCorrected = false;
               }
            }

            const float *pCrossfadeSrc = nullptr;
            size_t crossfadeStart = 0, totalCrossfadeLength = 0;
            if (i < mRecordingSchedule.mCrossfadeData.size())
            {
               // Do crossfading
               // The supplied crossfade samples are at the same rate as the track
               const auto &data = mRecordingSchedule.mCrossfadeData[i];
               totalCrossfadeLength = data.size();
               if (totalCrossfadeLength) {
                  crossfadeStart =
                     floor(mRecordingSchedule.Consumed() * mCaptureTracks[i]->GetRate());
                  if (crossfadeStart < totalCrossfadeLength)
                     pCrossfadeSrc = data.data() + crossfadeStart;
               }
            }

            wxASSERT(discarded <= avail);
            size_t toGet = avail - discarded;
            SampleBuffer temp;
            size_t size;
            sampleFormat format;
            if( mFactor == 1.0 )
            {
               // Take captured samples directly
               size = toGet;
               if (pCrossfadeSrc)
                  // Change to float for crossfade calculation
                  format = floatSample;
               else
                  format = trackFormat;
               temp.Allocate(size, format);
               const auto got =
                  mCaptureBuffers[i]->Get(temp.ptr(), format, toGet);
               // wxASSERT(got == toGet);
               // but we can't assert in this thread
               wxUnusedVar(got);
               if (double(size) > remainingSamples)
                  size = floor(remainingSamples);
            }
            else
            {
               size = lrint(toGet * mFactor);
               format = floatSample;
               SampleBuffer temp1(toGet, floatSample);
               temp.Allocate(size, format);
               const auto got =
                  mCaptureBuffers[i]->Get(temp1.ptr(), floatSample, toGet);
               // wxASSERT(got == toGet);
               // but we can't assert in this thread
               wxUnusedVar(got);
               /* we are re-sampling on the fly. The last resampling call
                * must flush any samples left in the rate conversion buffer
                * so that they get recorded
                */
               if (toGet > 0 ) {
                  if (double(toGet) > remainingSamples)
                     toGet = floor(remainingSamples);
                  const auto results =
                  mResample[i]->Process(mFactor, (float *)temp1.ptr(), toGet,
                                        !IsStreamActive(), (float *)temp.ptr(), size);
                  size = results.second;
               }
            }

            if (pCrossfadeSrc) {
               wxASSERT(format == floatSample);
               size_t crossfadeLength = std::min(size, totalCrossfadeLength - crossfadeStart);
               if (crossfadeLength) {
                  auto ratio = double(crossfadeStart) / totalCrossfadeLength;
                  auto ratioStep = 1.0 / totalCrossfadeLength;
                  auto pCrossfadeDst = (float*)temp.ptr();

                  // Crossfade loop here
                  for (size_t ii = 0; ii < crossfadeLength; ++ii) {
                     *pCrossfadeDst = ratio * *pCrossfadeDst + (1.0 - ratio) * *pCrossfadeSrc;
                     ++pCrossfadeSrc, ++pCrossfadeDst;
                     ratio += ratioStep;
                  }
               }
            }

            // Now append
            // see comment in second handler about guarantee
            newBlocks = mCaptureTracks[i]->Append(temp.ptr(), format, size, 1)
               || newBlocks;
         } // end loop over capture channels

         // Now update the recording schedule position
         mRecordingSchedule.mPosition += avail / mRate;
         mRecordingSchedule.mLatencyCorrected = latencyCorrected;

         auto pListener = GetListener();
         if (pListener && newBlocks)
            pListener->OnAudioIONewBlocks(&mCaptureTracks);

         if (pScope)
            pScope->Commit();
      }
      // end of record buffering
   },
   // handler
   [this] ( TenacityException *pException ) {
      if ( pException ) {
         // So that we don't attempt to fill the recording buffer again
         // before the main thread stops recording
         SetRecordingException();
         return ;
      }
      else
         // Don't want to intercept other exceptions (?)
         throw;
   },
   delayedHandler );
}

void AudioIoCallback::SetListener(
   const std::shared_ptr< AudioIOListener > &listener)
{
   if (IsBusy())
      return;

   mListener = listener;
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static void DoSoftwarePlaythrough(constSamplePtr inputBuffer,
                                  sampleFormat inputFormat,
                                  unsigned inputChannels,
                                  float *outputBuffer,
                                  unsigned long len)
{
   for (unsigned int i=0; i < inputChannels; i++) {
      auto inputPtr = inputBuffer + (i * SAMPLE_SIZE(inputFormat));

      SamplesToFloats(inputPtr, inputFormat,
         outputBuffer + i, len, inputChannels, 2);
   }

   // One mono input channel goes to both output channels...
   if (inputChannels == 1)
      for (int i=0; i < len; i++)
         outputBuffer[2*i + 1] = outputBuffer[2*i];
}

int audacityAudioCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          const PaStreamCallbackFlags statusFlags, void *userData )
{
   auto gAudioIO = AudioIO::Get();
   return gAudioIO->AudioCallback(
      static_cast<constSamplePtr>(inputBuffer),
      static_cast<float*>(outputBuffer), framesPerBuffer,
      timeInfo, statusFlags, userData);
}

// Stop recording if 'silence' is detected
// Start recording if sound detected.
//
//   By using CallAfter(), we can schedule the call to the toolbar
//   to run in the main GUI thread after the next event loop iteration.
//   That's important, because Pause() updates GUI, such as status bar,
//   and that should NOT happen in this audio non-gui thread.
void AudioIoCallback::CheckSoundActivatedRecordingLevel(
      float *inputSamples,
      unsigned long framesPerBuffer
   )
{
   // Quick returns if next to nothing to do.
   if( !mPauseRec )
      return;

   float maxPeak = 0.;
   for( unsigned long i = 0, cnt = framesPerBuffer * mNumCaptureChannels; i < cnt; ++i ) {
      float sample = fabs(*(inputSamples++));
      if (sample > maxPeak) {
         maxPeak = sample;
      }
   }

   bool bShouldBePaused = maxPeak < mSilenceLevel;
   if( bShouldBePaused != IsPaused() )
   {
      auto pListener = GetListener();
      if ( pListener )
         pListener->OnSoundActivationThreshold();
   }
}

// A function to apply the requested gain, fading up or down from the
// most recently applied gain.
void AudioIoCallback::AddToOutputChannel(
   unsigned int chan,
   float * outputMeterFloats,
   float * outputFloats,
   const float * tempBuf,
   bool drop,
   unsigned long len,
   WaveTrack& vt
)
{
   const auto numPlaybackChannels = mNumPlaybackChannels;

   float gain = vt.GetChannelGain(chan);
   if (drop || mForceFadeOut.load(std::memory_order_relaxed) || mPaused)
      gain = 0.0;

   // Output volume emulation: possibly copy meter samples, then
   // apply volume, then copy to the output buffer
   if (outputMeterFloats != outputFloats)
      for ( unsigned i = 0; i < len; ++i)
         outputMeterFloats[numPlaybackChannels*i+chan] +=
            gain*tempBuf[i];

   float oldGain = vt.GetOldChannelGain(chan);
   if( gain != oldGain )
      vt.SetOldChannelGain(chan, gain);
   // if no microfades, jump in volume.
   if( !mbMicroFades )
      oldGain =gain;
   assert(len > 0);

   // Linear interpolate.
   float deltaGain = (gain - oldGain) / len;
   for (unsigned i = 0; i < len; i++)
      outputFloats[numPlaybackChannels*i+chan] += (oldGain + deltaGain * i) *tempBuf[i];
};

// Limit values to -1.0..+1.0
void ClampBuffer(float * pBuffer, unsigned long len){
   for(unsigned i = 0; i < len; i++)
      pBuffer[i] = wxClip( pBuffer[i], -1.0f, 1.0f);
};

void AudioIoCallback::UpdateBuffers()
{
   mBuffersPrepared = false;
   auto channels = std::max(mNumPlaybackChannels, mNumCaptureChannels);

   if (IsStreamActive())
   {
      // Don't modify buffers if there's a stream active
      return;
   }

   unsigned long newBufferSize = GetConvertedLatencyPreference();
   auto& memoryManager = AudioMemoryManager::Get();

   mTemporaryBuffer.reset();
   mTemporaryBuffer = memoryManager.GetBuffer(newBufferSize);
   if (!mTemporaryBuffer)
   {
      memoryManager.CreateBuffer(newBufferSize);
      mTemporaryBuffer = memoryManager.GetBuffer(newBufferSize);
   }

   if (mTrackChannelsBuffer.size() < channels)
   {
      mTrackChannelsBuffer.resize(channels);
   }

   mScratchBufferAllocator.DeallocateAll();
   mScratchBuffers.resize(mNumPlaybackChannels);
   for (auto& buffer : mScratchBuffers)
   {
      buffer = mScratchBufferAllocator.Allocate(true, newBufferSize);
   }

   mBuffersPrepared = true;
}

// return true, IFF we have fully handled the callback.
//
// Mix and copy to PortAudio's output buffer
// from our intermediate playback buffers
//
bool AudioIoCallback::FillOutputBuffers(
   float *outputBuffer,
   unsigned long framesPerBuffer,
   float *outputMeterFloats
)
{
   // Debug
   assert(mBuffersPrepared);

   const auto numPlaybackTracks = mPlaybackTracks.size();
   const auto numPlaybackChannels = mNumPlaybackChannels;
   const auto numCaptureChannels = mNumCaptureChannels;

   mMaxFramesOutput = 0;

   // Quick returns if next to nothing to do.
   if (mStreamToken <= 0)
      return false;
   if( !outputBuffer )
      return false;
   if(numPlaybackChannels <= 0)
      return false;

   float *outputFloats = outputBuffer;

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT
   // While scrubbing, ignore seek requests
   if (mSeek && mPlaybackSchedule.Interactive())
      mSeek = 0.0;
#endif

   if (mSeek){
      mCallbackReturn = CallbackDoSeek();
      return true;
   }

   {
      auto pProject = mOwningProject.lock();
      RealtimeEffectManager::ProcessScope scope{ pProject.get() };

      bool selected = false;
      int group = 0;
      int chanCnt = 0;

      // Choose a common size to take from all ring buffers
      const auto toGet =
         std::min<size_t>(framesPerBuffer, GetCommonlyReadyPlayback());

      // The drop and dropQuickly booleans are so named for historical reasons.
      // JKC: The original code attempted to be faster by doing nothing on silenced audio.
      // This, IMHO, is 'premature optimisation'.  Instead clearer and cleaner code would
      // simply use a gain of 0.0 for silent audio and go on through to the stage of 
      // applying that 0.0 gain to the data mixed into the buffer.
      // Then (and only then) we would have if needed fast paths for:
      // - Applying a uniform gain of 0.0.
      // - Applying a uniform gain of 1.0.
      // - Applying some other uniform gain.
      // - Applying a linearly interpolated gain.
      // I would expect us not to need the fast paths, since linearly interpolated gain
      // is very cheap to process.

      bool drop = false;        // Track should become silent.
      bool dropQuickly = false; // Track has already been faded to silence.
      for (unsigned t = 0; t < numPlaybackTracks; t++)
      {
         WaveTrack* vt = mPlaybackTracks[t].get();
         mTrackChannelsBuffer[chanCnt] = vt;

         // TODO: more-than-two-channels
         auto nextTrack =
            t + 1 < numPlaybackTracks
               ? mPlaybackTracks[t + 1].get()
               : nullptr;

         // First and last channel in this group (for example left and right
         // channels of stereo).
         bool firstChannel = vt->IsLeader();
         bool lastChannel = !nextTrack || nextTrack->IsLeader();

         if ( firstChannel )
         {
            selected = vt->GetSelected();
            // IF mono THEN clear 'the other' channel.
            if ( lastChannel && (numPlaybackChannels>1))
            {
               // TODO: more-than-two-channels
               auto buf = mScratchBuffers[1];
               memset(buf, 0, framesPerBuffer * sizeof(float));
            }
            drop = TrackShouldBeSilent( *vt );
            dropQuickly = drop;
         }

         if( mbMicroFades )
            dropQuickly = dropQuickly && TrackHasBeenFadedOut( *vt );
            
         decltype(framesPerBuffer) len = 0;

         if (dropQuickly)
         {
            len = mPlaybackBuffers[t]->Discard(toGet);
            // keep going here.  
            // we may still need to issue a paComplete.
         }
         else
         {
            len = mPlaybackBuffers[t]->Get((samplePtr)mScratchBuffers[chanCnt],
                                                      floatSample,
                                                      toGet);
            // assert( len == toGet );
            if (len < framesPerBuffer)
            {
               // This used to happen normally at the end of non-looping
               // plays, but it can also be an anomalous case where the
               // supply from TrackBufferExchange fails to keep up with the
               // real-time demand in this thread (see bug 1932).  We
               // must supply something to the sound card, so pad it with
               // zeroes and not random garbage.
               memset((void*)&mScratchBuffers[chanCnt][len], 0,
                  (framesPerBuffer - len) * sizeof(float));
            }
            chanCnt++;
         }

         // PRL:  Bug1104:
         // There can be a difference of len in different loop passes if one channel
         // of a stereo track ends before the other!  Take a max!

         // PRL:  More recent rewrites of TrackBufferExchange should guarantee a
         // padding out of the ring buffers so that equal lengths are
         // available, so maxLen ought to increase from 0 only once
         mMaxFramesOutput = std::max(mMaxFramesOutput, len);

         if ( !lastChannel )
            continue;

         // Last channel of a track seen now
         len = mMaxFramesOutput;

         if( !dropQuickly && selected )
            len = scope.Process(group, chanCnt, mScratchBuffers.data(), len);
         group++;

         CallbackCheckCompletion(mCallbackReturn, len);
         if (dropQuickly) // no samples to process, they've been discarded
            continue;

         // Our channels aren't silent.  We need to pass their data on.
         //
         // Note that there are two kinds of channel count.
         // c and chanCnt are counting channels in the Tracks.
         // chan (and numPlayBackChannels) is counting output channels on the device.
         // chan = 0 is left channel
         // chan = 1 is right channel.
         //
         // Each channel in the tracks can output to more than one channel on the device.
         // For example mono channels output to both left and right output channels.
         if (len > 0) for (int c = 0; c < chanCnt; c++)
         {
            vt = mTrackChannelsBuffer[c];

            if (vt->GetChannelIgnoringPan() == Track::LeftChannel ||
                  vt->GetChannelIgnoringPan() == Track::MonoChannel )
               AddToOutputChannel( 0, outputMeterFloats, outputFloats,
                  mScratchBuffers[c], drop, len, *vt);

            if (vt->GetChannelIgnoringPan() == Track::RightChannel ||
                  vt->GetChannelIgnoringPan() == Track::MonoChannel  )
               AddToOutputChannel( 1, outputMeterFloats, outputFloats,
                  mScratchBuffers[c], drop, len, *vt);
         }

         chanCnt = 0;
      }

      // Poke: If there are no playback tracks, then the earlier check
      // about the time indicator being past the end won't happen;
      // do it here instead (but not if looping or scrubbing)
      if (numPlaybackTracks == 0)
         CallbackCheckCompletion(mCallbackReturn, 0);

      // assert( maxLen == toGet );
   }

   mLastPlaybackTimeMillis = ::wxGetUTCTimeMillis();

   ClampBuffer( outputFloats, framesPerBuffer*numPlaybackChannels );
   if (outputMeterFloats != outputFloats)
      ClampBuffer( outputMeterFloats, framesPerBuffer*numPlaybackChannels );

   return false;
}

void AudioIoCallback::UpdateTimePosition(unsigned long framesPerBuffer)
{
   // Quick returns if next to nothing to do.
   if (mStreamToken <= 0)
      return;

   // Update the position seen by drawing code
   if (mPlaybackSchedule.Interactive())
      // To do: do this in all cases and remove TrackTimeUpdate
      mPlaybackSchedule.SetTrackTime( mTimeQueue.Consumer( mMaxFramesOutput, mRate ) );
   else
      mPlaybackSchedule.TrackTimeUpdate( framesPerBuffer / mRate );
}

// return true, IFF we have fully handled the callback.
//
// Copy from PortAudio input buffers to our intermediate recording buffers.
//
void AudioIoCallback::DrainInputBuffers(
   constSamplePtr inputBuffer,
   unsigned long framesPerBuffer,
   const PaStreamCallbackFlags statusFlags,
   float * tempFloats
)
{
   const auto numPlaybackTracks = mPlaybackTracks.size();
   const auto numPlaybackChannels = mNumPlaybackChannels;
   const auto numCaptureChannels = mNumCaptureChannels;

   // Quick returns if next to nothing to do.
   if (mStreamToken <= 0)
      return;
   if( !inputBuffer )
      return;
   if( numCaptureChannels <= 0 )
      return;

   // If there are no playback tracks, and we are recording, then the
   // earlier checks for being past the end won't happen, so do it here.
   if (mPlaybackSchedule.PassIsComplete()) {
      mCallbackReturn = paComplete;
   }

   // The error likely from a too-busy CPU falling behind real-time data
   // is paInputOverflow
   bool inputError =
      (statusFlags & (paInputOverflow))
      && !(statusFlags & paPrimingOutput);

   // But it seems it's easy to get false positives, at least on Mac
   // So we have not decided to enable this extra detection yet in
   // production

   size_t len = framesPerBuffer;
   for(unsigned t = 0; t < numCaptureChannels; t++)
      len = std::min( len, mCaptureBuffers[t]->AvailForPut() );

   if (mSimulateRecordingErrors && 100LL * rand() < RAND_MAX)
      // Make spurious errors for purposes of testing the error
      // reporting
      len = 0;

   // A different symptom is that len < framesPerBuffer because
   // the other thread, executing TrackBufferExchange, isn't consuming fast
   // enough from mCaptureBuffers; maybe it's CPU-bound, or maybe the
   // storage device it writes is too slow
   if (mDetectDropouts &&
         ((mDetectUpstreamDropouts && inputError) ||
         len < framesPerBuffer) ) {
      // Assume that any good partial buffer should be written leftmost
      // and zeroes will be padded after; label the zeroes.
      auto start = mPlaybackSchedule.GetTrackTime() +
            len / mRate + mRecordingSchedule.mLatencyCorrection;
      auto duration = (framesPerBuffer - len) / mRate;
      auto pLast = mLostCaptureIntervals.empty()
         ? nullptr : &mLostCaptureIntervals.back();
      if (pLast &&
          fabs(pLast->first + pLast->second - start) < 0.5/mRate)
         // Make one bigger interval, not two butting intervals
         pLast->second = start + duration - pLast->first;
      else
         mLostCaptureIntervals.emplace_back( start, duration );
   }

   if (len < framesPerBuffer)
   {
      mLostSamples += (framesPerBuffer - len);
      std::cout << "lost " << (int)(framesPerBuffer - len) << " samples" << std::endl;
   }

   if (len <= 0) 
      return;

   // We have an ASSERT in the AudioIO constructor to alert us to 
   // possible issues with the (short*) cast.  We'd have a problem if
   // sizeof(short) > sizeof(float) since our buffers are sized for floats.
   for(unsigned t = 0; t < numCaptureChannels; t++) {

      // dmazzoni:
      // Un-interleave.  Ugly special-case code required because the
      // capture channels could be in three different sample formats;
      // it'd be nice to be able to call CopySamples, but it can't
      // handle multiplying by the gain and then clipping.  Bummer.

      switch(mCaptureFormat) {
         case floatSample: {
            auto inputFloats = (const float *)inputBuffer;
            for(unsigned i = 0; i < len; i++)
               tempFloats[i] =
                  inputFloats[numCaptureChannels*i+t];
         } break;
         case int24Sample:
            // We should never get here. Audacity's int24Sample format
            // is different from PortAudio's sample format and so we
            // make PortAudio return float samples when recording in
            // 24-bit samples.
            assert(false);
            break;
         case int16Sample: {
            auto inputShorts = (const short *)inputBuffer;
            short *tempShorts = (short *)tempFloats;
            for( unsigned i = 0; i < len; i++) {
               float tmp = inputShorts[numCaptureChannels*i+t];
               tmp = wxClip( -32768, tmp, 32767 );
               tempShorts[i] = (short)(tmp);
            }
         } break;
      } // switch

      // JKC: mCaptureFormat must be for samples with sizeof(float) or
      // fewer bytes (because tempFloats is sized for floats).  All 
      // formats are 2 or 4 bytes, so we are OK.
      //const auto put = // unused
         mCaptureBuffers[t]->Put(
            (samplePtr)tempFloats, mCaptureFormat, len);
      // assert(put == len);
      // but we can't assert in this thread
   }
}


#if 0
// Record the reported latency from PortAudio.
// TODO: Don't recalculate this with every callback?
// 01/21/2009:  Disabled until a better solution presents itself.
void OldCodeToCalculateLatency()
{
   // As of 06/17/2006, portaudio v19 returns inputBufferAdcTime set to
   // zero.  It is being worked on, but for now we just can't do much
   // but follow the leader.
   //
   // 08/27/2006: too inconsistent for now...just leave it a zero.
   //
   // 04/16/2008: Looks like si->inputLatency comes back with something useful though.
   // This rearranged logic uses si->inputLatency, but if PortAudio fixes inputBufferAdcTime,
   // this code won't have to be modified to use it.
   // Also avoids setting mLastRecordingOffset except when simultaneously playing and recording.
   //
   if (numCaptureChannels > 0 && numPlaybackChannels > 0) // simultaneously playing and recording
   {
      if (timeInfo->inputBufferAdcTime > 0)
         mLastRecordingOffset = timeInfo->inputBufferAdcTime - timeInfo->outputBufferDacTime;
      else if (mLastRecordingOffset == 0.0)
      {
         const PaStreamInfo* si = Pa_GetStreamInfo( mPortStreamV19 );
         mLastRecordingOffset = -si->inputLatency;
      }
   }
}
#endif


// return true, IFF we have fully handled the callback.
// Prime the output buffer with 0's, optionally adding in the playthrough.
void AudioIoCallback::DoPlaythrough(
      constSamplePtr inputBuffer,
      float *outputBuffer,
      unsigned long framesPerBuffer,
      float *outputMeterFloats
   )
{
   const auto numCaptureChannels = mNumCaptureChannels;
   const auto numPlaybackChannels = mNumPlaybackChannels;

   // Quick returns if next to nothing to do.
   if( !outputBuffer )
      return;
   if( numPlaybackChannels <= 0 )
      return;

   float *outputFloats = outputBuffer;
   for(unsigned i = 0; i < framesPerBuffer*numPlaybackChannels; i++)
      outputFloats[i] = 0.0;

   if (inputBuffer && mSoftwarePlaythrough) {
      DoSoftwarePlaythrough(inputBuffer, mCaptureFormat,
                              numCaptureChannels,
                              outputBuffer, framesPerBuffer);
   }

   // Copy the results to outputMeterFloats if necessary
   if (outputMeterFloats != outputFloats) {
      for (unsigned i = 0; i < framesPerBuffer*numPlaybackChannels; ++i) {
         outputMeterFloats[i] = outputFloats[i];
      }
   }
}

/* Send data to recording VU meter if applicable */
// Also computes rms
void AudioIoCallback::SendVuInputMeterData(
   const float *inputSamples,
   unsigned long framesPerBuffer
   )
{
   const auto numCaptureChannels = mNumCaptureChannels;

   auto pInputMeter = mInputMeter.lock();
   if ( !pInputMeter )
      return;
   if( pInputMeter->IsMeterDisabled())
      return;

   // get here if meters are actually live , and being updated
   /* It's critical that we don't update the meters while StopStream is
      * trying to stop PortAudio, otherwise it can lead to a freeze.  We use
      * two variables to synchronize:
      *   mUpdatingMeters tells StopStream when the callback is about to enter
      *     the code where it might update the meters, and
      *   mUpdateMeters is how the rest of the code tells the callback when it
      *     is allowed to actually do the updating.
      * Note that mUpdatingMeters must be set first to avoid a race condition.
      */
   //TODO use atomics instead.
   mUpdatingMeters = true;
   if (mUpdateMeters) {
         pInputMeter->UpdateDisplay(numCaptureChannels,
                                    framesPerBuffer,
                                    inputSamples);
   }
   mUpdatingMeters = false;
}

/* Send data to playback VU meter if applicable */
void AudioIoCallback::SendVuOutputMeterData(
   const float *outputMeterFloats,
   unsigned long framesPerBuffer)
{
   const auto numPlaybackChannels = mNumPlaybackChannels;

   auto pOutputMeter = mOutputMeter.lock();
   if (!pOutputMeter)
      return;
   if( pOutputMeter->IsMeterDisabled() )
      return;
   if( !outputMeterFloats) 
      return;

   // Get here if playback meter is live
   /* It's critical that we don't update the meters while StopStream is
      * trying to stop PortAudio, otherwise it can lead to a freeze.  We use
      * two variables to synchronize:
      *  mUpdatingMeters tells StopStream when the callback is about to enter
      *    the code where it might update the meters, and
      *  mUpdateMeters is how the rest of the code tells the callback when it
      *    is allowed to actually do the updating.
      * Note that mUpdatingMeters must be set first to avoid a race condition.
      */
   mUpdatingMeters = true;
   if (mUpdateMeters) {
      pOutputMeter->UpdateDisplay(numPlaybackChannels,
                                             framesPerBuffer,
                                             outputMeterFloats);

      //v Vaughan, 2011-02-25: Moved this update back to TrackPanel::OnTimer()
      //    as it helps with playback issues reported by Bill and noted on Bug 258.
      //    The problem there occurs if Software Playthrough is on.
      //    Could conditionally do the update here if Software Playthrough is off,
      //    and in TrackPanel::OnTimer() if Software Playthrough is on, but not now.
      // PRL 12 Jul 2015: and what was in TrackPanel::OnTimer is now handled by means of event
      // type EVT_TRACK_PANEL_TIMER
      //MixerBoard* pMixerBoard = mOwningProject->GetMixerBoard();
      //if (pMixerBoard)
      //   pMixerBoard->UpdateMeters(GetStreamTime(),
      //                              (pProj->GetControlToolBar()->GetLastPlayMode() == loopedPlay));
   }
   mUpdatingMeters = false;
}

unsigned AudioIoCallback::CountSoloingTracks(){
   const auto numPlaybackTracks = mPlaybackTracks.size();

   // MOVE_TO: CountSoloedTracks() function
   unsigned numSolo = 0;
   for(unsigned t = 0; t < numPlaybackTracks; t++ )
      if( mPlaybackTracks[t]->GetSolo() )
         numSolo++;
   auto range = Extensions();
   numSolo += std::accumulate(range.begin(), range.end(), 0,
      [](unsigned sum, auto &ext){
         return sum + ext.CountOtherSoloTracks(); });
   return numSolo;
}

// TODO: Consider making the two Track status functions into functions of
// WaveTrack.

// true IFF the track should be silent. 
// The track may not yet be silent, since it may still be
// fading out.
bool AudioIoCallback::TrackShouldBeSilent( const WaveTrack &wt )
{
   return mPaused || (!wt.GetSolo() && (
      // Cut if somebody else is soloing
      mbHasSoloTracks ||
      // Cut if we're muted (and not soloing)
      wt.GetMute()
   ));
}

// This is about micro-fades.
bool AudioIoCallback::TrackHasBeenFadedOut( const WaveTrack &wt )
{
   const auto channel = wt.GetChannelIgnoringPan();
   if ((channel == Track::LeftChannel  || channel == Track::MonoChannel) &&
      wt.GetOldChannelGain(0) != 0.0)
      return false;
   if ((channel == Track::RightChannel || channel == Track::MonoChannel) &&
      wt.GetOldChannelGain(1) != 0.0)
      return false;
   return true;
}

bool AudioIoCallback::AllTracksAlreadySilent()
{
   const bool dropAllQuickly = std::all_of(
      mPlaybackTracks.begin(), mPlaybackTracks.end(),
      [&]( const std::shared_ptr< WaveTrack > &vt )
         { return 
      TrackShouldBeSilent( *vt ) && 
      TrackHasBeenFadedOut( *vt ); }
   );
   return dropAllQuickly;
}

AudioIoCallback::AudioIoCallback()
{
   auto &factories = AudioIOExt::GetFactories();
   for (auto &factory: factories)
      if (auto pExt = factory(mPlaybackSchedule))
         mAudioIOExt.push_back( move(pExt) );
}


AudioIoCallback::~AudioIoCallback()
{
}


int AudioIoCallback::AudioCallback(
   constSamplePtr inputBuffer, float *outputBuffer,
   unsigned long framesPerBuffer,
   const PaStreamCallbackTimeInfo *timeInfo,
   const PaStreamCallbackFlags statusFlags, void * WXUNUSED(userData) )
{
   // Poll tracks for change of state.  User might click mute and solo buttons.
   mbHasSoloTracks = CountSoloingTracks() > 0 ;
   mCallbackReturn = paContinue;

   if (IsPaused()
       // PRL:  Why was this added?  Was it only because of the mysterious
       // initial leading zeroes, now solved by setting mStreamToken early?
       // JKC: I think it's used for the MIDI time cursor.  See comments
       // at head of file about AudioTime().
       || mStreamToken <= 0
       )
      mNumPauseFrames += framesPerBuffer;

   for( auto &ext : Extensions() ) {
      ext.ComputeOtherTimings(mRate,
         timeInfo,
         framesPerBuffer);
      ext.FillOtherBuffers(
         mRate, mNumPauseFrames, IsPaused(), mbHasSoloTracks);
   }

   // ------ MEMORY ALLOCATIONS -----------------------------------------------
   // tempFloats will be a reusable scratch pad for (possibly format converted)
   // audio data.  One temporary use is for the InputMeter data.
   const auto numPlaybackChannels = mNumPlaybackChannels;
   const auto numCaptureChannels = mNumCaptureChannels;
   std::unique_ptr<float> tempFloats(
      new float[framesPerBuffer * std::max(numCaptureChannels, numPlaybackChannels)]
   );
   // ----- END of MEMORY ALLOCATIONS ------------------------------------------

   if (inputBuffer && numCaptureChannels) {
      float *inputSamples;

      if (mCaptureFormat == floatSample) {
         inputSamples = (float *) inputBuffer;
      }
      else {
         SamplesToFloats(reinterpret_cast<constSamplePtr>(inputBuffer),
            mCaptureFormat, tempFloats.get(), framesPerBuffer * numCaptureChannels);
         inputSamples = tempFloats.get();
      }

      SendVuInputMeterData(
         inputSamples,
         framesPerBuffer);

      // This function may queue up a pause or resume.
      // TODO this is a bit dodgy as it toggles the Pause, and
      // relies on an idle event to have handled that, so could 
      // queue up multiple toggle requests and so do nothing.
      // Eventually it will sort itself out by random luck, but
      // the net effect is a delay in starting/stopping sound activated 
      // recording.
      CheckSoundActivatedRecordingLevel(
         inputSamples,
         framesPerBuffer);
   }

   // Even when paused, we do playthrough.
   // Initialise output buffer to zero or to playthrough data.
   // Initialise output meter values.
   DoPlaythrough(
      inputBuffer, 
      outputBuffer,
      framesPerBuffer,
      outputBuffer);

   // Test for no track audio to play (because we are paused and have faded out)
   if( mPaused &&  (( !mbMicroFades ) || AllTracksAlreadySilent() ))
      return mCallbackReturn;

   // To add track output to output (to play sound on speaker)
   // possible exit, if we were seeking.
   if( FillOutputBuffers(
         outputBuffer,
         framesPerBuffer,
         outputBuffer))
      return mCallbackReturn;

   // To move the cursor onwards.  (uses mMaxFramesOutput)
   UpdateTimePosition(framesPerBuffer);

   // To capture input into track (sound from microphone)
   DrainInputBuffers(
      inputBuffer,
      framesPerBuffer,
      statusFlags,
      tempFloats.get());

   SendVuOutputMeterData( outputBuffer, framesPerBuffer);

   return mCallbackReturn;
}

int AudioIoCallback::CallbackDoSeek()
{
   const int token = mStreamToken;
   std::lock_guard<std::mutex> locker(mSuspendAudioThread);
   if (token != mStreamToken)
      // This stream got destroyed while we waited for it
      return paAbort;

   const auto numPlaybackTracks = mPlaybackTracks.size();

   // Pause audio thread and wait for it to finish
   mAudioThreadTrackBufferExchangeLoopRunning = false;
   while( mAudioThreadTrackBufferExchangeLoopActive )
   {
      std::this_thread::sleep_for(std::chrono::milliseconds( 50 ));
   }

   // Calculate the NEW time position, in the PortAudio callback
   const auto time = mPlaybackSchedule.ClampTrackTime(
      mPlaybackSchedule.GetTrackTime() + mSeek );
   mPlaybackSchedule.SetTrackTime( time );
   mSeek = 0.0;

   mPlaybackSchedule.RealTimeInit( time );

   // Reset mixer positions and flush buffers for all tracks
   for (size_t i = 0; i < numPlaybackTracks; i++)
   {
      const bool skipping = true;
      mPlaybackMixers[i]->Reposition( time, skipping );
      const auto toDiscard =
         mPlaybackBuffers[i]->AvailForGet();
      const auto discarded =
         mPlaybackBuffers[i]->Discard( toDiscard );
      // assert( discarded == toDiscard );
      // but we can't assert in this thread
   }

   // Reload the ring buffers
   mAudioThreadShouldCallTrackBufferExchangeOnce = true;
   while( mAudioThreadShouldCallTrackBufferExchangeOnce )
   {
      std::this_thread::sleep_for(std::chrono::milliseconds( 50 ));
   }

   // Reenable the audio thread
   mAudioThreadTrackBufferExchangeLoopRunning = true;

   return paContinue;
}

void AudioIoCallback::CallbackCheckCompletion(
   int &callbackReturn, unsigned long len)
{
   if (mPaused)
      return;

   bool done = mPlaybackSchedule.PassIsComplete();
   if (!done)
      return;

   done =  mPlaybackSchedule.PlayingAtSpeed()
      // some leftover length allowed in this case
      || (mPlaybackSchedule.PlayingStraight() && len == 0);
   if(!done) 
      return;

   for( auto &ext : Extensions() )
      ext.SignalOtherCompletion();
   callbackReturn = paComplete;
}

auto AudioIoCallback::AudioIOExtIterator::operator *() const -> AudioIOExt &
{
   // Down-cast and dereference are safe because only AudioIOCallback
   // populates the array
   return *static_cast<AudioIOExt*>(mIterator->get());
}

void AudioIO::TimeQueue::Producer(
   const PlaybackSchedule &schedule, double rate, double scrubSpeed,
   size_t nSamples )
{
   if ( ! mData )
      // Recording only.  Don't fill the queue.
      return;

   // Don't check available space:  assume it is enough because of coordination
   // with RingBuffer.
   auto index = mTail.mIndex;
   auto time = mLastTime;
   auto remainder = mTail.mRemainder;
   auto space = TimeQueueGrainSize - remainder;

   while ( nSamples >= space ) {
      time = schedule.AdvancedTrackTime( time, space / rate, scrubSpeed );
      index = (index + 1) % mSize;
      mData[ index ] = time;
      nSamples -= space;
      remainder = 0;
      space = TimeQueueGrainSize;
   }

   // Last odd lot
   if ( nSamples > 0 )
      time = schedule.AdvancedTrackTime( time, nSamples / rate, scrubSpeed );

   mLastTime = time;
   mTail.mRemainder = remainder + nSamples;
   mTail.mIndex = index;
}

double AudioIO::TimeQueue::Consumer( size_t nSamples, double rate )
{
   if ( ! mData ) {
      // Recording only.  No scrub or playback time warp.  Don't use the queue.
      return ( mLastTime += nSamples / rate );
   }

   // Don't check available space:  assume it is enough because of coordination
   // with RingBuffer.
   auto remainder = mHead.mRemainder;
   auto space = TimeQueueGrainSize - remainder;
   if ( nSamples >= space ) {
      remainder = 0,
      mHead.mIndex = (mHead.mIndex + 1) % mSize,
      nSamples -= space;
      if ( nSamples >= TimeQueueGrainSize )
         mHead.mIndex =
            (mHead.mIndex + ( nSamples / TimeQueueGrainSize ) ) % mSize,
         nSamples %= TimeQueueGrainSize;
   }
   mHead.mRemainder = remainder + nSamples;
   return mData[ mHead.mIndex ];
}

bool AudioIO::IsCapturing() const
{
   // Includes a test of mTime, used in the main thread
   return IsStreamActive() &&
      GetNumCaptureChannels() > 0 &&
      mPlaybackSchedule.GetTrackTime() >=
         mPlaybackSchedule.mT0 + mRecordingSchedule.mPreRoll;
}
