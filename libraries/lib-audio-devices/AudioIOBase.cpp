/**********************************************************************

Audacity: A Digital Audio Editor

AudioIOBase.cpp

Paul Licameli split from AudioIO.cpp

**********************************************************************/


#include "AudioIOBase.h"

#include "Meter.h"
#include "Prefs.h"

#include <portaudio.h>
#include <iostream>
#include <sstream>

#ifdef EXPERIMENTAL_MIDI_OUT
#include <portmidi.h>
#endif

int AudioIOBase::mCachedPlaybackIndex = -1;
std::vector<long> AudioIOBase::mCachedPlaybackRates;
int AudioIOBase::mCachedCaptureIndex = -1;
std::vector<long> AudioIOBase::mCachedCaptureRates;
std::vector<long> AudioIOBase::mCachedSampleRates;
double AudioIOBase::mCachedBestRateIn = 0.0;

const int AudioIOBase::StandardRates[] = {
   8000,
   11025,
   16000,
   22050,
   32000,
   44100,
   48000,
   88200,
   96000,
   176400,
   192000,
   352800,
   384000
};

const int AudioIOBase::NumStandardRates = sizeof(AudioIOBase::StandardRates) / sizeof(int);

const int AudioIOBase::RatesToTry[] = {
   8000,
   9600,
   11025,
   12000,
   15000,
   16000,
   22050,
   24000,
   32000,
   44100,
   48000,
   88200,
   96000,
   176400,
   192000,
   352800,
   384000
};
const int AudioIOBase::NumRatesToTry = sizeof(AudioIOBase::RatesToTry) / sizeof(int);

std::string AudioIOBase::DeviceName(const PaDeviceInfo* info)
{
   std::string infoName = std::string(info->name);

   return infoName;
}

std::string AudioIOBase::HostName(const PaDeviceInfo* info)
{
   std::string hostapiName = std::string(Pa_GetHostApiInfo(info->hostApi)->name);

   return hostapiName;
}

std::unique_ptr<AudioIOBase> AudioIOBase::ugAudioIO;

AudioIOBase *AudioIOBase::Get()
{
   return ugAudioIO.get();
}

AudioIOBase::~AudioIOBase() = default;

void AudioIOBase::HandleDeviceChange()
{
   // This should not happen, but it would screw things up if it did.
   // Vaughan, 2010-10-08: But it *did* happen, due to a bug, and nobody
   // caught it because this method just returned. Added assert().
   assert(!IsStreamActive());
   if (IsStreamActive())
      return;

   // get the selected record and playback devices
   const int playDeviceNum = getPlayDevIndex();
   const int recDeviceNum = getRecordDevIndex();

   // If no change needed, return
   if (mCachedPlaybackIndex == playDeviceNum &&
       mCachedCaptureIndex == recDeviceNum)
       return;

   // cache playback/capture rates
   mCachedPlaybackRates = GetSupportedPlaybackRates(playDeviceNum);
   mCachedCaptureRates = GetSupportedCaptureRates(recDeviceNum);
   mCachedSampleRates = GetSupportedSampleRates(playDeviceNum, recDeviceNum);
   mCachedPlaybackIndex = playDeviceNum;
   mCachedCaptureIndex = recDeviceNum;
   mCachedBestRateIn = 0.0;

}

void AudioIOBase::SetCaptureMeter(
   TenacityProject *project, const std::weak_ptr<Meter> &wMeter)
{
   if (( mOwningProject ) && ( mOwningProject != project))
      return;

   auto meter = wMeter.lock();
   if (meter)
   {
      mInputMeter = meter;
      meter->Reset(mRate, true);
   }
   else
      mInputMeter.reset();
}

void AudioIOBase::SetPlaybackMeter(
   TenacityProject *project, const std::weak_ptr<Meter> &wMeter)
{
   if (( mOwningProject ) && ( mOwningProject != project))
      return;

   auto meter = wMeter.lock();
   if (meter)
   {
      mOutputMeter = meter;
      meter->Reset(mRate, true);
   }
   else
      mOutputMeter.reset();
}

bool AudioIOBase::IsPaused() const
{
   return mPaused;
}

bool AudioIOBase::IsBusy() const
{
   if (mStreamToken != 0)
      return true;

   return false;
}

bool AudioIOBase::IsStreamActive() const
{
   bool isActive = false;
   // JKC: Not reporting any Pa error, but that looks OK.
   if( mPortStreamV19 )
      isActive = (Pa_IsStreamActive( mPortStreamV19 ) > 0);

#ifdef EXPERIMENTAL_MIDI_OUT
   if( mMidiStreamActive && !mMidiOutputComplete )
      isActive = true;
#endif
   return isActive;
}

bool AudioIOBase::IsStreamActive(int token) const
{
   return (this->IsStreamActive() && this->IsAudioTokenActive(token));
}

bool AudioIOBase::IsAudioTokenActive(int token) const
{
   return ( token > 0 && token == mStreamToken );
}

bool AudioIOBase::IsMonitoring() const
{
   return ( mPortStreamV19 && mStreamToken==0 );
}

std::vector<long> AudioIOBase::GetSupportedPlaybackRates(int devIndex, double rate)
{
   if (devIndex == -1)
   {  // weren't given a device index, get the prefs / default one
      devIndex = getPlayDevIndex();
   }

   // Check if we can use the cached rates
   if (mCachedPlaybackIndex != -1 && devIndex == mCachedPlaybackIndex
         && (rate == 0.0 || make_iterator_range(mCachedPlaybackRates).contains(rate)))
   {
      return mCachedPlaybackRates;
   }

   std::vector<long> supported;
   int irate = (int)rate;
   const PaDeviceInfo* devInfo = NULL;
   int i;

   devInfo = Pa_GetDeviceInfo(devIndex);

   if (!devInfo)
   {
      std::cout << "GetSupportedPlaybackRates() Could not get device info!" << std::endl;
      return supported;
   }

   // LLL: Remove when a proper method of determining actual supported
   //      DirectSound rate is devised.
   const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(devInfo->hostApi);
   bool isDirectSound = (hostInfo && hostInfo->type == paDirectSound);

   PaStreamParameters pars;

   pars.device = devIndex;
   pars.channelCount = 1;
   pars.sampleFormat = paFloat32;
   pars.suggestedLatency = devInfo->defaultHighOutputLatency;
   pars.hostApiSpecificStreamInfo = NULL;

   // JKC: PortAudio Errors handled OK here.  No need to report them
   for (i = 0; i < NumRatesToTry; i++)
   {
      // LLL: Remove when a proper method of determining actual supported
      //      DirectSound rate is devised.
      if (!(isDirectSound && RatesToTry[i] > 200000)){
         if (Pa_IsFormatSupported(NULL, &pars, RatesToTry[i]) == 0)
            supported.push_back(RatesToTry[i]);
         Pa_Sleep( 10 );// There are ALSA drivers that don't like being probed
         // too quickly.
      }
   }

   if (irate != 0 && !make_iterator_range(supported).contains(irate))
   {
      // LLL: Remove when a proper method of determining actual supported
      //      DirectSound rate is devised.
      if (!(isDirectSound && RatesToTry[i] > 200000))
         if (Pa_IsFormatSupported(NULL, &pars, irate) == 0)
            supported.push_back(irate);
   }

   return supported;
}

std::vector<long> AudioIOBase::GetSupportedCaptureRates(int devIndex, double rate)
{
   if (devIndex == -1)
   {  // not given a device, look up in prefs / default
      devIndex = getRecordDevIndex();
   }

   // Check if we can use the cached rates
   if (mCachedCaptureIndex != -1 && devIndex == mCachedCaptureIndex
         && (rate == 0.0 || make_iterator_range(mCachedCaptureRates).contains(rate)))
   {
      return mCachedCaptureRates;
   }

   std::vector<long> supported;
   int irate = (int)rate;
   const PaDeviceInfo* devInfo = NULL;
   int i;

   devInfo = Pa_GetDeviceInfo(devIndex);

   if (!devInfo)
   {
      std::cout << "GetSupportedCaptureRates() Could not get device info!" << std::endl;
      return supported;
   }

   auto latencyDuration = AudioIOLatencyDuration.Read();
   // Why not defaulting to 2 as elsewhere?
   auto recordChannels = AudioIORecordChannels.ReadWithDefault(1);

   // LLL: Remove when a proper method of determining actual supported
   //      DirectSound rate is devised.
   const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(devInfo->hostApi);
   bool isDirectSound = (hostInfo && hostInfo->type == paDirectSound);

   PaStreamParameters pars;

   pars.device = devIndex;
   pars.channelCount = recordChannels;
   pars.sampleFormat = paFloat32;
   pars.suggestedLatency = latencyDuration / 1000.0;
   pars.hostApiSpecificStreamInfo = NULL;

   for (i = 0; i < NumRatesToTry; i++)
   {
      // LLL: Remove when a proper method of determining actual supported
      //      DirectSound rate is devised.
      if (!(isDirectSound && RatesToTry[i] > 200000))
      {
         if (Pa_IsFormatSupported(&pars, NULL, RatesToTry[i]) == 0)
            supported.push_back(RatesToTry[i]);
         Pa_Sleep( 10 );// There are ALSA drivers that don't like being probed
         // too quickly.
      }
   }

   if (irate != 0 && !make_iterator_range(supported).contains(irate))
   {
      // LLL: Remove when a proper method of determining actual supported
      //      DirectSound rate is devised.
      if (!(isDirectSound && RatesToTry[i] > 200000))
         if (Pa_IsFormatSupported(&pars, NULL, irate) == 0)
            supported.push_back(irate);
   }

   return supported;
}

std::vector<long> AudioIOBase::GetSupportedSampleRates(
   int playDevice, int recDevice, double rate)
{
   // Not given device indices, look up prefs
   if (playDevice == -1) {
      playDevice = getPlayDevIndex();
   }
   if (recDevice == -1) {
      recDevice = getRecordDevIndex();
   }

   // Check if we can use the cached rates
   if (mCachedPlaybackIndex != -1 && mCachedCaptureIndex != -1 &&
         playDevice == mCachedPlaybackIndex &&
         recDevice == mCachedCaptureIndex &&
         (rate == 0.0 || make_iterator_range(mCachedSampleRates).contains(rate)))
   {
      return mCachedSampleRates;
   }

   auto playback = GetSupportedPlaybackRates(playDevice, rate);
   auto capture = GetSupportedCaptureRates(recDevice, rate);
   int i;

   // Return only sample rates which are in both arrays
   std::vector<long> result;

   for (i = 0; i < (int)playback.size(); i++)
      if (make_iterator_range(capture).contains(playback[i]))
         result.push_back(playback[i]);

   // If this yields no results, use the default sample rates nevertheless
/*   if (result.empty())
   {
      for (i = 0; i < NumStandardRates; i++)
         result.push_back(StandardRates[i]);
   }*/

   return result;
}

/** \todo: should this take into account PortAudio's value for
 * PaDeviceInfo::defaultSampleRate? In principal this should let us work out
 * which rates are "real" and which resampled in the drivers, and so prefer
 * the real rates. */
int AudioIOBase::GetOptimalSupportedSampleRate()
{
   auto rates = GetSupportedSampleRates();

   if (make_iterator_range(rates).contains(48000))
      return 48000;

   if (make_iterator_range(rates).contains(44100))
      return 44100;

   // if there are no supported rates, the next bit crashes. So check first,
   // and give them a "sensible" value if there are no valid values. They
   // will still get an error later, but with any luck may have changed
   // something by then. It's no worse than having an invalid default rate
   // stored in the preferences, which we don't check for
   if (rates.empty()) return 48000;

   return rates.back();
}

int AudioIOBase::getPlayDevIndex(const std::string &devNameArg)
{
   std::string devName(devNameArg);
   // if we don't get given a device, look up the preferences
   if (devName.empty())
      devName = AudioIOPlaybackDevice.Read();

   auto hostName = AudioIOHost.Read();
   PaHostApiIndex hostCnt = Pa_GetHostApiCount();
   PaHostApiIndex hostNum;
   for (hostNum = 0; hostNum < hostCnt; hostNum++)
   {
      const PaHostApiInfo *hinfo = Pa_GetHostApiInfo(hostNum);
      if (hinfo && std::string(std::string(hinfo->name)) == hostName)
      {
         for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; hostDevice++)
         {
            PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(hostNum, hostDevice);

            const PaDeviceInfo *dinfo = Pa_GetDeviceInfo(deviceNum);
            if (dinfo && DeviceName(dinfo) == devName && dinfo->maxOutputChannels > 0 )
            {
               // this device name matches the stored one, and works.
               // So we say this is the answer and return it
               return deviceNum;
            }
         }

         // The device wasn't found so use the default for this host.
         // LL:  At this point, preferences and active no longer match.
         return hinfo->defaultOutputDevice;
      }
   }

   // The host wasn't found, so use the default output device.
   // FIXME: TRAP_ERR PaErrorCode not handled well (this code is similar to input code
   // and the input side has more comments.)

   PaDeviceIndex deviceNum = Pa_GetDefaultOutputDevice();

   // Sometimes PortAudio returns -1 if it cannot find a suitable default
   // device, so we just use the first one available
   //
   // LL:  At this point, preferences and active no longer match
   //
   //      And I can't imagine how far we'll get specifying an "invalid" index later
   //      on...are we certain "0" even exists?
   if (deviceNum < 0) {
      assert(false);
      deviceNum = 0;
   }

   return deviceNum;
}

int AudioIOBase::getRecordDevIndex(const std::string &devNameArg)
{
   std::string devName(devNameArg);
   // if we don't get given a device, look up the preferences
   if (devName.empty())
      devName = AudioIORecordingDevice.Read();

   auto hostName = AudioIOHost.Read();
   PaHostApiIndex hostCnt = Pa_GetHostApiCount();
   PaHostApiIndex hostNum;
   for (hostNum = 0; hostNum < hostCnt; hostNum++)
   {
      const PaHostApiInfo *hinfo = Pa_GetHostApiInfo(hostNum);
      if (hinfo && std::string(std::string(hinfo->name)) == hostName)
      {
         for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; hostDevice++)
         {
            PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(hostNum, hostDevice);

            const PaDeviceInfo *dinfo = Pa_GetDeviceInfo(deviceNum);
            if (dinfo && DeviceName(dinfo) == devName && dinfo->maxInputChannels > 0 )
            {
               // this device name matches the stored one, and works.
               // So we say this is the answer and return it
               return deviceNum;
            }
         }

         // The device wasn't found so use the default for this host.
         // LL:  At this point, preferences and active no longer match.
         return hinfo->defaultInputDevice;
      }
   }

   // The host wasn't found, so use the default input device.
   // FIXME: TRAP_ERR PaErrorCode not handled well in getRecordDevIndex()
   PaDeviceIndex deviceNum = Pa_GetDefaultInputDevice();

   // Sometimes PortAudio returns -1 if it cannot find a suitable default
   // device, so we just use the first one available
   // PortAudio has an error reporting function.  We should log/report the error?
   //
   // LL:  At this point, preferences and active no longer match
   //
   //      And I can't imagine how far we'll get specifying an "invalid" index later
   //      on...are we certain "0" even exists?
   if (deviceNum < 0) {
      // JKC: This ASSERT will happen if you run with no config file
      // This happens once.  Config file will exist on the next run.
      // TODO: Look into this a bit more.  Could be relevant to blank Device Toolbar.
      assert(false);
      deviceNum = 0;
   }

   return deviceNum;
}

std::string AudioIOBase::GetDeviceInfo()
{
   std::ostringstream s;

   if (IsStreamActive()) {
      return XO("Stream is active ... unable to gather information.\n")
         .Translation()
         .ToStdString(); // ANERRUPTION: Remove std::string conversion
   }

   // FIXME: TRAP_ERR PaErrorCode not handled.  3 instances in GetDeviceInfo().
   int recDeviceNum = Pa_GetDefaultInputDevice();
   int playDeviceNum = Pa_GetDefaultOutputDevice();
   int cnt = Pa_GetDeviceCount();

   // PRL:  why only into the log?
   std::cout << "PortAudio reports " << cnt << " audio devices" << std::endl;

   s << std::string("==============================") << std::endl;
   s << XO("Default recording device number: %d").Format( recDeviceNum ).Translation() << std::endl;
   s << XO("Default playback device number: %d").Format( playDeviceNum).Translation() << std::endl;

   auto recDevice = AudioIORecordingDevice.Read();
   auto playDevice = AudioIOPlaybackDevice.Read();
   int j;

   // This gets info on all available audio devices (input and output)
   if (cnt <= 0) {
      s << XO("No devices found").Translation() << std::endl;
      return s.str();
   }

   const PaDeviceInfo* info;

   for (j = 0; j < cnt; j++) {
      s << std::string("==============================") << std::endl;

      info = Pa_GetDeviceInfo(j);
      if (!info) {
         s << XO("Device info unavailable for: %d").Format( j ).Translation() << std::endl;
         continue;
      }

      std::string name = DeviceName(info);
      s << XO("Device ID: %d").Format( j ).Translation() << std::endl;
      s << XO("Device name: %s").Format( name ).Translation() << std::endl;
      s << XO("Host name: %s").Format( HostName(info) ).Translation() << std::endl;
      s << XO("Recording channels: %d").Format( info->maxInputChannels ).Translation() << std::endl;
      s << XO("Playback channels: %d").Format( info->maxOutputChannels ).Translation() << std::endl;
      s << XO("Low Recording Latency: %g").Format( info->defaultLowInputLatency ).Translation() << std::endl;
      s << XO("Low Playback Latency: %g").Format( info->defaultLowOutputLatency ).Translation() << std::endl;
      s << XO("High Recording Latency: %g").Format( info->defaultHighInputLatency ).Translation() << std::endl;
      s << XO("High Playback Latency: %g").Format( info->defaultHighOutputLatency ).Translation() << std::endl;

      auto rates = GetSupportedPlaybackRates(j, 0.0);

      /* i18n-hint: Supported, meaning made available by the system */
      s << XO("Supported Rates:").Translation() << std::endl;
      for (int k = 0; k < (int) rates.size(); k++) {
         s << std::string("    ") << (int)rates[k] << std::endl;
      }

      if (name == playDevice && info->maxOutputChannels > 0)
         playDeviceNum = j;

      if (name == recDevice && info->maxInputChannels > 0)
         recDeviceNum = j;

      // Sometimes PortAudio returns -1 if it cannot find a suitable default
      // device, so we just use the first one available
      if (recDeviceNum < 0 && info->maxInputChannels > 0){
         recDeviceNum = j;
      }
      if (playDeviceNum < 0 && info->maxOutputChannels > 0){
         playDeviceNum = j;
      }
   }

   bool haveRecDevice = (recDeviceNum >= 0);
   bool havePlayDevice = (playDeviceNum >= 0);

   s << std::string("==============================") << std::endl;
   if (haveRecDevice)
      s << XO("Selected recording device: %d - %s").Format( recDeviceNum, recDevice ).Translation() << std::endl;
   else
      s << XO("No recording device found for '%s'.").Format( recDevice ).Translation() << std::endl;

   if (havePlayDevice)
      s << XO("Selected playback device: %d - %s").Format( playDeviceNum, playDevice ).Translation() << std::endl;
   else
      s << XO("No playback device found for '%s'.").Format( playDevice ).Translation() << std::endl;

   std::vector<long> supportedSampleRates;

   if (havePlayDevice && haveRecDevice) {
      supportedSampleRates = GetSupportedSampleRates(playDeviceNum, recDeviceNum);

      s << XO("Supported Rates:").Translation() << std::endl;
      for (int k = 0; k < (int) supportedSampleRates.size(); k++) {
         s << std::string("    ") << (int)supportedSampleRates[k] << std::endl;
      }
   }
   else {
      s << XO("Cannot check mutual sample rates without both devices.").Translation() << std::endl;
      return s.str();
   }

   return s.str();
}

#ifdef EXPERIMENTAL_MIDI_OUT
// FIXME: When EXPERIMENTAL_MIDI_IN is added (eventually) this should also be enabled -- Poke
std::string AudioIOBase::GetMidiDeviceInfo()
{
   std::ostringstream s;

   if (IsStreamActive()) {
      return XO("Stream is active ... unable to gather information.\n")
         .Translation();
   }


   // XXX: May need to trap errors as with the normal device info
   int recDeviceNum = Pm_GetDefaultInputDeviceID();
   int playDeviceNum = Pm_GetDefaultOutputDeviceID();
   int cnt = Pm_CountDevices();

   // PRL:  why only into the log?
   std::cout << "PortMidi reports " << cnt << " MIDI devices" << std::endl;

   s << std::string("==============================") << std::endl;
   s << XO("Default recording device number: %d").Format( recDeviceNum ).Translation() << std::endl;
   s << XO("Default playback device number: %d").Format( playDeviceNum ).Translation() << std::endl;

   std::string recDevice = gPrefs->Read(wxT("/MidiIO/RecordingDevice"), wxT(""));
   std::string playDevice = gPrefs->Read(wxT("/MidiIO/PlaybackDevice"), wxT(""));

   // This gets info on all available audio devices (input and output)
   if (cnt <= 0) {
      s << XO("No devices found").Translation() << std::endl;
      return s.str();
   }

   for (int i = 0; i < cnt; i++) {
      s << std::string("==============================") << std::endl;

      const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
      if (!info) {
         s << XO("Device info unavailable for: %d").Format( i ).Translation() << std::endl;
         continue;
      }

      std::string name = std::string(info->name);
      std::string hostName = std::string(info->interf);

      s << XO("Device ID: %d").Format( i ).Translation() << std::endl;
      s << XO("Device name: %s").Format( name ).Translation() << std::endl;
      s << XO("Host name: %s").Format( hostName ).Translation() << std::endl;
      /* i18n-hint: Supported, meaning made available by the system */
      s << XO("Supports output: %d").Format( info->output ).Translation() << std::endl;
      /* i18n-hint: Supported, meaning made available by the system */
      s << XO("Supports input: %d").Format( info->input ).Translation() << std::endl;
      s << XO("Opened: %d").Format( info->opened ).Translation() << std::endl;

      if (name == playDevice && info->output)
         playDeviceNum = i;

      if (name == recDevice && info->input)
         recDeviceNum = i;

      // XXX: This is only done because the same was applied with PortAudio
      // If PortMidi returns -1 for the default device, use the first one
      if (recDeviceNum < 0 && info->input){
         recDeviceNum = i;
      }
      if (playDeviceNum < 0 && info->output){
         playDeviceNum = i;
      }
   }

   bool haveRecDevice = (recDeviceNum >= 0);
   bool havePlayDevice = (playDeviceNum >= 0);

   s << std::string("==============================") << std::endl;
   if (haveRecDevice)
      s << XO("Selected MIDI recording device: %d - %s").Format( recDeviceNum, recDevice ).Translation() << std::endl;
   else
      s << XO("No MIDI recording device found for '%s'.").Format( recDevice ).Translation() << std::endl;

   if (havePlayDevice)
      s << XO("Selected MIDI playback device: %d - %s").Format( playDeviceNum, playDevice ).Translation() << std::endl;
   else
      s << XO("No MIDI playback device found for '%s'.").Format( playDevice ).Translation() << std::endl;

   // Mention our conditional compilation flags for Alpha only
#ifdef IS_ALPHA

   // Not internationalizing these alpha-only messages
   s << std::string("==============================") << std::endl;
#ifdef EXPERIMENTAL_MIDI_OUT
   s << "EXPERIMENTAL_MIDI_OUT is enabled" << std::endl;
#else
   s << "EXPERIMENTAL_MIDI_OUT is NOT enabled" << std::endl;
#endif
#ifdef EXPERIMENTAL_MIDI_IN
   s << "EXPERIMENTAL_MIDI_IN is enabled" << std::endl;
#else
   s << "EXPERIMENTAL_MIDI_IN is NOT enabled" << std::endl;
#endif

#endif

   return s.str();
}
#endif

StringSetting AudioIOHost{
   L"/AudioIO/Host", L"" };
DoubleSetting AudioIOLatencyCorrection{
   L"/AudioIO/LatencyCorrection", -130.0 };
DoubleSetting AudioIOLatencyDuration{
   L"/AudioIO/LatencyDuration", 512.0 };
ChoiceSetting AudioIOLatencyUnit{
   L"/AudioIO/LatencyUnit",
   {
      ByColumns,
      { XO("samples"), XO("milliseconds") },
      { L"samples",    L"milliseconds"    }
   },
};
StringSetting AudioIOPlaybackDevice{
   L"/AudioIO/PlaybackDevice", L"" };
IntSetting AudioIORecordChannels{
   L"/AudioIO/RecordChannels", 2 };
StringSetting AudioIORecordingDevice{
   L"/AudioIO/RecordingDevice", L"" };
StringSetting AudioIORecordingSource{
   L"/AudioIO/RecordingSource", L"" };
IntSetting AudioIORecordingSourceIndex{
   L"/AudioIO/RecordingSourceIndex", -1 };
