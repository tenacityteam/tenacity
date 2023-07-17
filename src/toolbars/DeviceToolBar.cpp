/**********************************************************************

  Audacity: A Digital Audio Editor

  DeviceToolBar.cpp

  Dominic Mazzoni

*******************************************************************//*!

\class DeviceToolBar
\brief A toobar to allow easier changing of input and output devices .

*//*******************************************************************/



#include "DeviceToolBar.h"
#include "ToolManager.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/choice.h>
#include <wx/event.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/tooltip.h>
#endif

// Tenacity libraries
#include <lib-audio-devices/AudioIOBase.h>
#include <lib-project/Project.h>
#include <lib-preferences/Prefs.h>

#include "../TrackPanel.h"

#include "../AColor.h"
#include "../theme/AllThemeResources.h"
#include "../ImageManipulation.h"
#include "../KeyboardCapture.h"
#include "../widgets/Grabber.h"
#include "DeviceManager.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/Grabber.h"

#if wxUSE_ACCESSIBILITY
#include "../widgets/WindowAccessible.h"
#endif

IMPLEMENT_CLASS(DeviceToolBar, ToolBar);

////////////////////////////////////////////////////////////
/// Methods for DeviceToolBar
////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(DeviceToolBar, ToolBar)
   EVT_CHOICE(wxID_ANY, DeviceToolBar::OnChoice)
   EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, DeviceToolBar::OnCaptureKey)
END_EVENT_TABLE()

static int DeviceToolbarPrefsID()
{
   static int value = wxNewId();
   return value;
}

//Standard constructor
DeviceToolBar::DeviceToolBar( TenacityProject &project )
: ToolBar( project, DeviceBarID, XO("Device"), wxT("Device"), true )
{
   DeviceManager::Instance()->Bind( EVT_RESCANNED_DEVICES,
      &DeviceToolBar::OnRescannedDevices, this );
}

DeviceToolBar::~DeviceToolBar()
{
}

DeviceToolBar &DeviceToolBar::Get( TenacityProject &project )
{
   auto &toolManager = ToolManager::Get( project );
   return *static_cast<DeviceToolBar*>( toolManager.GetToolBar(DeviceBarID) );
}

const DeviceToolBar &DeviceToolBar::Get( const TenacityProject &project )
{
   return Get( const_cast<TenacityProject&>( project )) ;
}

void DeviceToolBar::Create(wxWindow *parent)
{
   ToolBar::Create(parent);

   // Simulate a size event to set initial meter placement/size
   wxSizeEvent event(GetSize(), GetId());
   event.SetEventObject(this);
   GetEventHandler()->ProcessEvent(event);
}

void DeviceToolBar::DeinitChildren()
{
   mInput         = NULL;
   mOutput        = NULL;
   mInputChannels = NULL;
   mHost          = NULL;
}

void DeviceToolBar::Populate()
{
   SetBackgroundColour( theTheme.Colour( clrMedium  ) );
   DeinitChildren();

   // Hosts
   mHost = safenew wxChoice(this,
                            wxID_ANY,
                            wxDefaultPosition,
                            wxDefaultSize);
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mHost->SetAccessible(safenew WindowAccessible(mHost));
#endif
   Add(mHost, 15, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 1);

   // Input device
   Add(safenew AStaticBitmap(this,
                             wxID_ANY,
                             theTheme.Bitmap(bmpMic)), 0, wxALIGN_CENTER_VERTICAL);
   mInput = safenew wxChoice(this,
                             wxID_ANY,
                             wxDefaultPosition,
                             wxDefaultSize);
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mInput->SetAccessible(safenew WindowAccessible(mInput));
#endif
   Add(mInput, 30, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 1);

   // Input channels
   mInputChannels = safenew wxChoice(this,
                                     wxID_ANY,
                                     wxDefaultPosition,
                                     wxDefaultSize);
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mInputChannels->SetAccessible(safenew WindowAccessible(mInputChannels));
#endif
   Add(mInputChannels, 20, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 1);

   // Output device
   Add(safenew AStaticBitmap(this,
                             wxID_ANY,
                             theTheme.Bitmap(bmpSpeaker)), 0, wxALIGN_CENTER_VERTICAL);
   mOutput = safenew wxChoice(this,
                              wxID_ANY,
                              wxDefaultPosition,
                              wxDefaultSize);
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mOutput->SetAccessible(safenew WindowAccessible(mOutput));
#endif
   Add(mOutput, 30, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 1);

   mHost->Bind(wxEVT_SET_FOCUS,
               &DeviceToolBar::OnFocus,
               this);
   mHost->Bind(wxEVT_KILL_FOCUS,
               &DeviceToolBar::OnFocus,
               this);
   mOutput->Bind(wxEVT_SET_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);
   mOutput->Bind(wxEVT_KILL_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);
   mInput->Bind(wxEVT_SET_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);
   mInput->Bind(wxEVT_KILL_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);
   mInputChannels->Bind(wxEVT_SET_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);
   mInputChannels->Bind(wxEVT_KILL_FOCUS,
                 &DeviceToolBar::OnFocus,
                 this);

   SetNames();

   RefillCombos();
}

void DeviceToolBar::OnFocus(wxFocusEvent &event)
{
   KeyboardCapture::OnFocus( *this, event );
}

void DeviceToolBar::OnCaptureKey(wxCommandEvent &event)
{
   wxKeyEvent *kevent = (wxKeyEvent *)event.GetEventObject();
   int keyCode = kevent->GetKeyCode();

   // Pass UP/DOWN/LEFT/RIGHT through for input/output choice
   if (FindFocus() == mInput && (keyCode == WXK_LEFT || keyCode == WXK_RIGHT
                                 || keyCode == WXK_UP || keyCode == WXK_DOWN)) {
      return;
   }

   if (FindFocus() == mOutput && (keyCode == WXK_LEFT || keyCode == WXK_RIGHT
                                 || keyCode == WXK_UP || keyCode == WXK_DOWN)) {
      return;
   }
   event.Skip();

   return;
}

void DeviceToolBar::UpdatePrefs()
{
   wxString desc;
   const std::vector<Device> &inDevices  = DeviceManager::Instance()->GetInputDevices();
   const std::vector<Device> &outDevices = DeviceManager::Instance()->GetOutputDevices();


   int hostSelectionIndex = mHost->GetSelection();
   wxString oldHost = hostSelectionIndex >= 0 ? mHost->GetString(hostSelectionIndex) :
                                                wxString{};
   std::string hostName = AudioIOHost.Read().ToStdString();

   // if the prefs host name doesn't match the one displayed, it changed
   // in another project's DeviceToolBar, so we need to repopulate everything.
   if (oldHost != hostName)
      FillHostDevices();

   auto devName = AudioIORecordingDevice.Read();
   desc = devName;

   if (mInput->GetStringSelection() != desc &&
       mInput->FindString(desc) != wxNOT_FOUND) {
      mInput->SetStringSelection(desc);
      FillInputChannels();
   } else if (mInput->GetStringSelection() != desc && mInput->GetCount()) {
      for (size_t i = 0; i < inDevices.size(); i++) {
         if (inDevices[i].GetHostName() == hostName &&
             inDevices[i].GetName()     == mInput->GetString(0))
         {
            // use the default.  It should exist but check just in case, falling back on the 0 index.
            Device* defaultDevice = DeviceManager::Instance()->GetDefaultInputDevice(inDevices[i].GetHostIndex());
            if (defaultDevice)
            {
               mInput->SetStringSelection(defaultDevice->GetName());
               SetDevices(defaultDevice, nullptr);
            } else {
               //use the first item (0th index) if we have no familiar devices
               mInput->SetSelection(0);
               SetDevices(&inDevices[i], nullptr);
            }
            break;
         }
      }
   }

   devName = AudioIOPlaybackDevice.Read();
   desc = devName;

   if (mOutput->GetStringSelection() != desc &&
       mOutput->FindString(desc) != wxNOT_FOUND) {
      mOutput->SetStringSelection(desc);
   } else if (mOutput->GetStringSelection() != desc &&
              mOutput->GetCount()) {
      for (size_t i = 0; i < outDevices.size(); i++) {
         if (outDevices[i].GetHostName()   == hostName &&
             outDevices[i].GetName() == mOutput->GetString(0)) {
            // use the default.  It should exist but check just in case, falling back on the 0 index.
            Device* defaultMap = DeviceManager::Instance()->GetDefaultOutputDevice(outDevices[i].GetHostIndex());
            if (defaultMap) {
               mOutput->SetStringSelection(defaultMap->GetName());
               SetDevices(NULL, defaultMap);
            } else {
               //use the first item (0th index) if we have no familiar devices
               mOutput->SetSelection(0);
               SetDevices(NULL, &outDevices[i]);
            }
            break;
         }
      }
   }

   long oldChannels;
   oldChannels = mInputChannels->GetSelection() + 1;
   auto newChannels = AudioIORecordChannels.ReadWithDefault(0);
   if (newChannels > 0 && oldChannels != newChannels)
      mInputChannels->SetSelection(newChannels - 1);

   if (!hostName.empty() && mHost->GetStringSelection() != hostName)
      mHost->SetStringSelection(hostName);

   RegenerateTooltips();

   // Set label to pull in language change
   SetLabel(XO("Device"));

   // Give base class a chance
   ToolBar::UpdatePrefs();

   Layout();
   Refresh();
}

void DeviceToolBar::UpdateSelectedPrefs( int id )
{
   if (id == DeviceToolbarPrefsID())
      UpdatePrefs();
   ToolBar::UpdateSelectedPrefs( id );
}


void DeviceToolBar::EnableDisableButtons()
{
   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO) {
      // we allow changes when monitoring, but not when recording
      bool audioStreamActive = gAudioIO->IsStreamActive() && !gAudioIO->IsMonitoring();

      // Here we should relinquish focus
      if (audioStreamActive) {
         wxWindow *focus = wxWindow::FindFocus();
         if (focus == mHost || focus == mInput || focus == mOutput || focus == mInputChannels)
            TrackPanel::Get( mProject ).SetFocus();
      }

      mHost->Enable(!audioStreamActive);
      mInput->Enable(!audioStreamActive);
      mOutput->Enable(!audioStreamActive);
      mInputChannels->Enable(!audioStreamActive);
   }
}

void DeviceToolBar::SetNames()
{
   /* i18n-hint: (noun) It's the device used for playback.*/
   mOutput->SetName(_("Playback Device"));
   /* i18n-hint: (noun) It's the device used for recording.*/
   mInput->SetName(_("Recording Device"));
   mHost->SetName(_("Audio Host"));
   mInputChannels->SetName(_("Recording Channels"));
}

void DeviceToolBar::RegenerateTooltips()
{
#if wxUSE_TOOLTIPS
   SetNames();
   mOutput->SetToolTip(mOutput->GetName() + wxT(" - ") + mOutput->GetStringSelection());
   mInput->SetToolTip(mInput->GetName() + wxT(" - ") + mInput->GetStringSelection());
   mHost->SetToolTip(mHost->GetName() + wxT(" - ") + mHost->GetStringSelection());
   mInputChannels->SetToolTip(mInputChannels->GetName() + wxT(" - ") + mInputChannels->GetStringSelection());
#endif
}

void DeviceToolBar::RefillCombos()
{
   FillHosts();
   FillHostDevices();
   FillInputChannels();
   // make the device display selection reflect the prefs if they exist
   UpdatePrefs();
}

void DeviceToolBar::FillHosts()
{
   const std::vector<Device> &inDevices = DeviceManager::Instance()->GetInputDevices();
   const std::vector<Device> &outDevices = DeviceManager::Instance()->GetOutputDevices();

   wxArrayString hosts;

   // go over our lists add the host to the list if it isn't there yet

   for (auto & device : inDevices) {
      if (!make_iterator_range(hosts).contains(device.GetHostName())) {
         hosts.push_back(device.GetHostName());
      }
   }

   for (auto & device : outDevices) {
      if (!make_iterator_range(hosts).contains(device.GetHostName())) {
         hosts.push_back(device.GetHostName());
      }
   }

   mHost->Clear();
   mHost->Append(hosts);

   if (hosts.size() == 0) {
      mHost->Enable(false);
   }

   mHost->SetMinSize(wxSize(50, wxDefaultCoord));
}

void DeviceToolBar::FillHostDevices()
{
   const std::vector<Device> &inDevices  = DeviceManager::Instance()->GetInputDevices();
   const std::vector<Device> &outDevices = DeviceManager::Instance()->GetOutputDevices();

   //read what is in the prefs
   auto host = AudioIOHost.Read();
   int foundHostIndex = -1;

   // if the host is not in the hosts combo then we rescanned.
   // set it to blank so we search for another host.
   if (mHost->FindString(host) == wxNOT_FOUND) {
      host = wxT("");
   }

   for (auto & device : outDevices) {
      if (device.GetHostName() == host) {
         foundHostIndex = device.GetHostIndex();
         break;
      }
   }

   if (foundHostIndex == -1) {
      for (auto & device : inDevices) {
         if (device.GetHostName() == host) {
            foundHostIndex = device.GetHostIndex();
            break;
         }
      }
   }

   // If no host was found based on the prefs device host, load the first available one
   if (foundHostIndex == -1) {
      if (outDevices.size()) {
         foundHostIndex = outDevices[0].GetHostIndex();
      }
      else if (inDevices.size()) {
         foundHostIndex = inDevices[0].GetHostIndex();
      }
   }

   // Make sure in/out are clear in case no host was found
   mInput->Clear();
   mOutput->Clear();

   // If we still have no host it means no devices, in which case do nothing.
   if (foundHostIndex == -1) {
      return;
   }

   // Repopulate the Input/Output device list available to the user
   for (auto & device : inDevices) {
      if (foundHostIndex == device.GetHostIndex()) {
         mInput->Append(device.GetName());
         if (host.empty()) {
            host = device.GetHostName();
            AudioIOHost.Write(host);
            mHost->SetStringSelection(host);
         }
      }
   }
   mInput->Enable(mInput->GetCount() ? true : false);

   mInput->SetMinSize(wxSize(50, wxDefaultCoord));

   for (auto & device : outDevices) {
      if (foundHostIndex == device.GetHostIndex()) {
         mOutput->Append(device.GetName());
         if (host.empty()) {
            host = device.GetHostName();
            AudioIOHost.Write(host);
            gPrefs->Flush();
            mHost->SetStringSelection(host);
         }
      }
   }
   mOutput->Enable(mOutput->GetCount() ? true : false);

   mOutput->SetMinSize(wxSize(50, wxDefaultCoord));

   // The setting of the Device is left up to OnChoice
}

void DeviceToolBar::FillInputChannels()
{
   const std::vector<Device> &inDevices = DeviceManager::Instance()->GetInputDevices();
   auto host = AudioIOHost.Read();
   auto device = AudioIORecordingDevice.Read();
   long newChannels;

   auto oldChannels = AudioIORecordChannels.Read();
   mInputChannels->Clear();
   for (auto & dev: inDevices)
   {
      if (device == dev.GetName() &&
          host   == dev.GetHostName())
      {

         // add one selection for each channel of this source
         for (size_t j = 0; j < (unsigned int) dev.GetNumChannels(); j++) {
            wxString name;

            if (j == 0) {
               name = _("1 (Mono) Recording Channel");
            }
            else if (j == 1) {
               name = _("2 (Stereo) Recording Channels");
            }
            else {
               name = wxString::Format(wxT("%d"), (int) j + 1);
            }
            mInputChannels->Append(name);
         }
         newChannels = dev.GetNumChannels();
         if (oldChannels <= newChannels && oldChannels >= 1) {
            newChannels = oldChannels;
         }
         if (newChannels >= 1) {
            mInputChannels->SetSelection(newChannels - 1);
         }
         AudioIORecordChannels.Write(newChannels);
         break;
      }
   }
   mInputChannels->Enable(mInputChannels->GetCount() ? true : false);

   mInputChannels->SetMinSize(wxSize(50, wxDefaultCoord));
}

void DeviceToolBar::OnRescannedDevices( wxEvent &event )
{
   event.Skip();
   // Hosts may have disappeared or appeared so a complete repopulate is needed.
   RefillCombos();
}

//return 1 if host changed, 0 otherwise.
int DeviceToolBar::ChangeHost()
{
   int hostSelectionIndex;
   hostSelectionIndex = mHost->GetSelection();

   auto oldHost = AudioIOHost.Read();
   wxString newHost = hostSelectionIndex >= 0 ? mHost->GetString(hostSelectionIndex) :
                                                oldHost;

   if (oldHost == newHost)
      return 0;

   //change the host and switch to correct devices.
   AudioIOHost.Write(newHost);
   gPrefs->Flush();

   // populate the devices
   FillHostDevices();

   return 1;
}

void DeviceToolBar::SetDevices(const Device* in, const Device* out)
{
   if (in) {
      AudioIORecordingDevice.Write(in->GetName());
      gPrefs->Flush();

      FillInputChannels();
   }

   if (out) {
      AudioIOPlaybackDevice.Write(out->GetName());
      gPrefs->Flush();
   }
}

void DeviceToolBar::ChangeDevice(bool isInput)
{
   int newIndex = -1;
   wxChoice *combo = isInput ? mInput :mOutput;
   size_t i;

   int selectionIndex  = combo->GetSelection();
   auto host = AudioIOHost.Read();
   const std::vector<Device>& devices = isInput ? DeviceManager::Instance()->GetInputDevices()
                                                : DeviceManager::Instance()->GetOutputDevices();

   // Find device indices for input and output
   if (selectionIndex >= 0 ) {
      std::string newDevice = combo->GetStringSelection().ToStdString();
      for (i = 0; i < devices.size(); ++i) {
         std::string name = devices[i].GetName();
         if (name == newDevice && devices[i].GetHostName() == host) {
            newIndex = i;
         }
      }
   }

   if (newIndex < 0) {
      wxLogDebug(wxT("DeviceToolBar::OnChoice(): couldn't find device indices"));
      return;
   }

   SetDevices(isInput ? &devices[newIndex] : nullptr,
              isInput ? nullptr            : &devices[newIndex]);
}

void DeviceToolBar::OnChoice(wxCommandEvent &event)
{
   wxObject *eventObject = event.GetEventObject();
   //if we've changed hosts, we've handled the device switching already.
   if (eventObject == mHost) {
      ChangeHost();
   } else if (eventObject == mInputChannels) {
      int channelsSelectionIndex = mInputChannels->GetSelection();
      if (channelsSelectionIndex >= 0)
         AudioIORecordChannels.Write(channelsSelectionIndex + 1);
   } else if (eventObject == mInput) {
      ChangeDevice(true);
   }
   else if (eventObject == mOutput) {
      ChangeDevice(false);
   }

   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO) {
      // We cannot have gotten here if gAudioIO->IsAudioTokenActive(),
      // per the setting of AudioIONotBusyFlag and AudioIOBusyFlag in
      // TenacityProject::GetUpdateFlags().
      // However, we can have an invalid audio token (so IsAudioTokenActive()
      // is false), but be monitoring.
      // If monitoring, have to stop the stream, so HandleDeviceChange() can work.
      // We could disable the Preferences command while monitoring, i.e.,
      // set AudioIONotBusyFlag/AudioIOBusyFlag according to monitoring, as well.
      // Instead allow it because unlike recording, for example, monitoring
      // is not clearly something that should prohibit changing device.
      // TODO: We *could* be smarter in this method and call HandleDeviceChange()
      // only when the device choices actually changed. True of lots of prefs!
      // As is, we always stop monitoring before handling the device change.
      if (gAudioIO->IsMonitoring())
      {
         gAudioIO->StopStream();
         while (gAudioIO->IsBusy())
            wxMilliSleep(100);
      }
      gAudioIO->HandleDeviceChange();
   }

   PrefsListener::Broadcast(DeviceToolbarPrefsID());
}

void DeviceToolBar::ShowInputDialog()
{
   ShowComboDialog(mInput, XO("Select Recording Device"));
}
void DeviceToolBar::ShowOutputDialog()
{
   ShowComboDialog(mOutput, XO("Select Playback Device"));
}
void DeviceToolBar::ShowHostDialog()
{
   ShowComboDialog(mHost, XO("Select Audio Host"));
}
void DeviceToolBar::ShowChannelsDialog()
{
   ShowComboDialog(mInputChannels, XO("Select Recording Channels"));
}

void DeviceToolBar::ShowComboDialog(wxChoice *combo, const TranslatableString &title)
{
   if (!combo || combo->GetCount() == 0) {
      AudacityMessageBox( XO("Device information is not available.") );
      return;
   }

}

static RegisteredToolbarFactory factory{ DeviceBarID,
   []( TenacityProject &project ){
      return ToolBar::Holder{ safenew DeviceToolBar{ project } }; }
};

namespace {
AttachedToolBarMenuItem sAttachment{
   /* i18n-hint: Clicking this menu item shows the toolbar
      that manages devices */
   DeviceBarID, wxT("ShowDeviceTB"), XXO("&Device Toolbar")
};
}

