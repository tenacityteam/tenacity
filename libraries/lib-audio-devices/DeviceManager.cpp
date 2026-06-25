/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2010 Audacity Team
   Michael Chinen

******************************************************************/


#include "DeviceManager.h"

#include <wx/log.h>
#include <thread>



#include "portaudio.h"

#include "AudioIOBase.h"

#include "DeviceChange.h" // for HAVE_DEVICE_CHANGE

DeviceManager DeviceManager::dm;

/// Gets the singleton instance
DeviceManager* DeviceManager::Instance()
{
   return &dm;
}

const std::vector<DeviceSourceMap> &DeviceManager::GetInputDeviceMaps()
{
   if (!m_inited)
      Init();
   return mInputDeviceSourceMaps;
}
const std::vector<DeviceSourceMap> &DeviceManager::GetOutputDeviceMaps()
{
   if (!m_inited)
      Init();
   return mOutputDeviceSourceMaps;
}


wxString MakeDeviceSourceString(const DeviceSourceMap *map)
{
   wxString ret;
   ret = map->deviceString;
   if (map->totalSources > 1)
      ret += wxT(": ") + map->sourceString;

   return ret;
}

DeviceSourceMap* DeviceManager::GetDefaultDevice(int hostIndex, int isInput)
{
   if (hostIndex < 0 || hostIndex >= Pa_GetHostApiCount()) {
      return NULL;
   }

   const struct PaHostApiInfo *apiinfo = Pa_GetHostApiInfo(hostIndex);   // get info on API
   std::vector<DeviceSourceMap> & maps = isInput ? mInputDeviceSourceMaps : mOutputDeviceSourceMaps;
   size_t i;
   int targetDevice = isInput ? apiinfo->defaultInputDevice : apiinfo->defaultOutputDevice;

   for (i = 0; i < maps.size(); i++) {
      if (maps[i].deviceIndex == targetDevice)
         return &maps[i];
   }

   wxLogDebug(wxT("GetDefaultDevice() no default device"));
   return NULL;
}

DeviceSourceMap* DeviceManager::GetDefaultOutputDevice(int hostIndex)
{
   return GetDefaultDevice(hostIndex, 0);
}
DeviceSourceMap* DeviceManager::GetDefaultInputDevice(int hostIndex)
{
   return GetDefaultDevice(hostIndex, 1);
}

//--------------- Device Enumeration --------------------------

static void AddDevice(int deviceIndex, std::vector<DeviceSourceMap> *maps, int isInput)
{
   const PaDeviceInfo *info = Pa_GetDeviceInfo(deviceIndex);
   if (!info)
      return;

   DeviceSourceMap map;
   map.deviceIndex  = deviceIndex;
   map.hostIndex    = info->hostApi;
   map.deviceString = wxSafeConvertMB2WX(info->name);
   map.hostString   = wxSafeConvertMB2WX(Pa_GetHostApiInfo(info->hostApi)->name);
   map.numChannels  = isInput ? info->maxInputChannels : info->maxOutputChannels;
   map.sourceIndex  = 0;
   map.totalSources = 0;
   maps->push_back(std::move(map));
}

/// Gets a NEW list of devices by terminating and restarting portaudio
/// Assumes that DeviceManager is only used on the main thread.
void DeviceManager::Rescan()
{
   // get rid of the previous scan info
   this->mInputDeviceSourceMaps.clear();
   this->mOutputDeviceSourceMaps.clear();

   // if we are doing a second scan then restart portaudio to get NEW devices
   if (m_inited) {
      // check to see if there is a stream open - can happen if monitoring,
      // but otherwise Rescan() should not be available to the user.
      auto gAudioIO = AudioIOBase::Get();
      if (gAudioIO) {
         if (gAudioIO->IsMonitoring())
         {
            using namespace std::chrono;
            gAudioIO->StopStream();
            while (gAudioIO->IsBusy())
               std::this_thread::sleep_for(100ms);
         }
      }

      // restart portaudio - this updates the device list
      // FIXME: TRAP_ERR restarting PortAudio
      Pa_Terminate();
      Pa_Initialize();
   }

   // FIXME: TRAP_ERR PaErrorCode not handled in ReScan()
   int nDevices = Pa_GetDeviceCount();

   for (int i = 0; i < nDevices; i++) {
      const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
      if (!info)
         continue;
      if (info->maxOutputChannels > 0)
         AddDevice(i, &mOutputDeviceSourceMaps, 0);
      if (info->maxInputChannels > 0)
         AddDevice(i, &mInputDeviceSourceMaps, 1);
   }

   // If this was not an initial scan update each device toolbar.
   if ( m_inited )
      Publish(DeviceChangeMessage::Rescan);

   m_inited = true;
   mRescanTime = std::chrono::steady_clock::now();
}


std::chrono::duration<float> DeviceManager::GetTimeSinceRescan() {
   auto now = std::chrono::steady_clock::now();
   auto dur = std::chrono::duration_cast<std::chrono::duration<float>>(now - mRescanTime);
   return dur;
}


//private constructor - Singleton.
DeviceManager::DeviceManager()
#if defined(EXPERIMENTAL_DEVICE_CHANGE_HANDLER)
#if defined(HAVE_DEVICE_CHANGE)
:  DeviceChangeHandler()
#endif
#endif
{
   m_inited = false;
   mRescanTime = std::chrono::steady_clock::now();
}

DeviceManager::~DeviceManager()
{

}

void DeviceManager::Init()
{
    Rescan();

#if defined(EXPERIMENTAL_DEVICE_CHANGE_HANDLER)
#if defined(HAVE_DEVICE_CHANGE)
   DeviceChangeHandler::Enable(true);
#endif
#endif
}

#if defined(EXPERIMENTAL_DEVICE_CHANGE_HANDLER)
#if defined(HAVE_DEVICE_CHANGE)
void DeviceManager::DeviceChangeNotification()
{
   Rescan();
   return;
}
#endif
#endif
