/**********************************************************************

  Audacity: A Digital Audio Editor

  MeterToolBar.cpp

  Dominic Mazzoni
  Leland Lucius

  See MeterToolBar.h for details

*******************************************************************//*!

\class MeterToolBar
\brief A ToolBar that holds the VU Meter

*//*******************************************************************/



#include "MeterToolBar.h"
#include "ToolBar.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/event.h>
#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/tooltip.h>
#endif

#include <wx/window.h>
#include <wx/gbsizer.h>

#include "../theme/AllThemeResources.h"
#include "../ProjectAudioIO.h"
#include "../prefs/PrefsDialog.h"
#include "../ProjectWindows.h"
#include "../AudioIO.h"

#include "../widgets/AButton.h"
#include "../widgets/MeterPanel.h"

// Tenacity libraries
#include <lib-audio-devices/DeviceManager.h>

#if wxUSE_ACCESSIBILITY
#include "../widgets/WindowAccessible.h"
#endif

////////////////////////////////////////////////////////////
/// Methods for MeterToolBar
////////////////////////////////////////////////////////////
enum {
   OnHostID,
   OnMonitorID,
   OnRecPreferencesID,
   OnPlayPreferencesID,
   OnDeviceID
};

//Standard constructor
MeterToolBar::MeterToolBar(TenacityProject &project, int type)
: ToolBar(project, type, XO("Combined Meter"), wxT("CombinedMeter"), true)
{
   Bind(wxEVT_SIZE, &MeterToolBar::OnSize, this);
   Bind(wxEVT_MENU, &MeterToolBar::OnHost, this, OnHostID);
   Bind(wxEVT_MENU, &MeterToolBar::OnMonitor, this, OnMonitorID);
   Bind(wxEVT_MENU, &MeterToolBar::OnRecPreferences, this, OnRecPreferencesID);
   Bind(wxEVT_MENU, &MeterToolBar::OnPlayPreferences, this, OnPlayPreferencesID);

   if( mType == RecordMeterBarID ){
      mWhichMeters = kWithRecordMeter;
      mLabel = XO("Recording Meter");
      mSection = wxT("RecordMeter");
   } else if( mType == PlayMeterBarID ){
      mWhichMeters = kWithPlayMeter;
      mLabel = XO("Playback Meter");
      mSection = wxT("PlayMeter");
   } else {
      mWhichMeters = kWithPlayMeter | kWithRecordMeter;
   }
   mSizer = NULL;
   mPlayMeter = NULL;
   mRecordMeter = NULL;
   mChannelsMenu = safenew wxMenu();
   mIOMenu = safenew wxMenu();

}

MeterToolBar::~MeterToolBar()
{
   mChannelsMenu=nullptr;
   delete mIOMenu;
   delete mChannelsMenu;
}

void MeterToolBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);

   UpdatePrefs();

   // Simulate a size event to set initial meter placement/size
   wxSizeEvent dummy;
   OnSize(dummy);
}

void MeterToolBar::ReCreateButtons()
{
   MeterPanel::State playState{ false }, recordState{ false };

   auto &projectAudioIO = ProjectAudioIO::Get( mProject );
   if (mPlayMeter &&
      projectAudioIO.GetPlaybackMeter() == mPlayMeter->GetMeter())
   {
      playState = mPlayMeter->SaveState();
      projectAudioIO.SetPlaybackMeter( nullptr );
   }

   if (mRecordMeter &&
      projectAudioIO.GetCaptureMeter() == mRecordMeter->GetMeter())
   {
      recordState = mRecordMeter->SaveState();
      projectAudioIO.SetCaptureMeter( nullptr );
   }

   ToolBar::ReCreateButtons();

   mPlayMeter->RestoreState(playState);
   if( playState.mSaved  ){
      projectAudioIO.SetPlaybackMeter( mPlayMeter->GetMeter() );
   }
   mRecordMeter->RestoreState(recordState);
   if( recordState.mSaved ){
      projectAudioIO.SetCaptureMeter( mRecordMeter->GetMeter() );
   }
}

void MeterToolBar::Populate()
{
   SetBackgroundColour( theTheme.Colour( clrMedium  ) );

   Add((mSizer = safenew wxGridBagSizer()), 1, wxEXPAND);

   if( mWhichMeters & kWithRecordMeter ){
      //Add Button
      AButton *mInButton = MeterToolBar::MakeButton(this,
         bmpRecoloredUpSmall, bmpRecoloredDownSmall, bmpRecoloredUpHiliteSmall, bmpRecoloredHiliteSmall,
         bmpMic, bmpMic, bmpMic,
         wxWindowID(ID_INPUT_BUTTON),
         wxDefaultPosition, false,
         theTheme.ImageSize( bmpRecoloredUpSmall ));
      mInButton->SetLabel(XO("Input"));
      mInButton->SetFocusRect( mInButton->GetClientRect().Deflate( 1, 1 ) );
      mInButton->SetToolTip(XO("Input settings"));
      mSizer->Add( mInButton, wxGBPosition( 0, (mWhichMeters & kWithPlayMeter)?2:0 ), wxGBSpan(2,1), wxALIGN_CENTER_VERTICAL );

      //JKC: Record on left, playback on right.  Left to right flow
      //(maybe we should do it differently for Arabic language :-)  )
      mRecordMeter = safenew MeterPanel( &mProject,
                                this,
                                wxID_ANY,
                                true,
                                wxDefaultPosition,
                                wxSize( 260, 28 ) );
      /* i18n-hint: (noun) The meter that shows the loudness of the audio being recorded.*/
      mRecordMeter->SetName( XO("Record Meter"));
      /* i18n-hint: (noun) The meter that shows the loudness of the audio being recorded.
       This is the name used in screen reader software, where having 'Meter' first
       apparently is helpful to partially sighted people.  */
      mRecordMeter->SetLabel( XO("Meter-Record") );
      mSizer->Add( mRecordMeter, wxGBPosition( 0,  (mWhichMeters & kWithPlayMeter)?3:1 ), wxDefaultSpan, wxEXPAND );

      Bind(wxEVT_BUTTON, &MeterToolBar::OnInputButton, this, ID_INPUT_BUTTON);
   }

   if( mWhichMeters & kWithPlayMeter ){
      //Add Button
      AButton *mOutButton = MeterToolBar::MakeButton(this,
         bmpRecoloredUpSmall, bmpRecoloredDownSmall, bmpRecoloredUpHiliteSmall, bmpRecoloredHiliteSmall,
         bmpSpeaker, bmpSpeaker, bmpSpeaker,
         wxWindowID(ID_OUTPUT_BUTTON),
         wxDefaultPosition, false,
         theTheme.ImageSize( bmpRecoloredUpSmall ));
      mOutButton->SetLabel(XO("Output"));
      mOutButton->SetFocusRect( mOutButton->GetClientRect().Deflate( 1, 1 ) );
      mOutButton->SetToolTip(XO("Output settings"));
      mSizer->Add( mOutButton, wxGBPosition( 0, 0 ), wxGBSpan(2,1), wxALIGN_CENTER_VERTICAL );

      mPlayMeter = safenew MeterPanel( &mProject,
                              this,
                              wxID_ANY,
                              false,
                              wxDefaultPosition,
                              wxSize( 260, 28 ) );
      /* i18n-hint: (noun) The meter that shows the loudness of the audio playing.*/
      mPlayMeter->SetName( XO("Play Meter"));
      /* i18n-hint: (noun) The meter that shows the loudness of the audio playing.
       This is the name used in screen reader software, where having 'Meter' first
       apparently is helpful to partially sighted people.  */
      mPlayMeter->SetLabel( XO("Meter-Play"));

      mOutSizer->Add( mPlayMeter, wxGBPosition( 0, 2 ), wxDefaultSpan, wxEXPAND );
      Bind(wxEVT_BUTTON, &MeterToolBar::OnOutputButton, this, ID_OUTPUT_BUTTON);
   }

   RegenerateTooltips();
}

void MeterToolBar::UpdatePrefs()
{
   RegenerateTooltips();

   // Set label to pull in language change
   SetLabel(XO("Meter"));

   // Give base class a chance
   ToolBar::UpdatePrefs();
}

void MeterToolBar::RegenerateTooltips()
{
#if wxUSE_TOOLTIPS
   if( mPlayMeter )
      mPlayMeter->SetToolTip( XO("Playback Level") );
   if( mRecordMeter )
      mRecordMeter->SetToolTip( XO("Recording Level") );
#endif
}

void MeterToolBar::OnSize( wxSizeEvent & event) ///* event */ )
{
   event.Skip();
   int width, height;

   // We can be resized before populating...protect against it
   if( !mSizer ) {
      return;
   }

   // Update the layout
   Layout();

   // Get the usable area
   wxSize sz = GetSizer()->GetSize();
   width = sz.x; height = sz.y;
   width -= 27;// get button size

   int nMeters = 
      ((mRecordMeter ==NULL) ? 0:1) +
      ((mPlayMeter ==NULL) ? 0:1);

   bool bHorizontal = ( width > height );
   bool bEndToEnd   = ( nMeters > 1 ) && std::min( width, height ) < (60 * nMeters);

   // Default location for second meter
   wxGBPosition pos( 0, 0 );
   // If 2 meters, share the height or width.
   if( nMeters > 1 ){
      if( bHorizontal ^ bEndToEnd ){
         height /= nMeters;
         pos = wxGBPosition( 1, 0 );
      } else {
         width /= nMeters;
         pos = wxGBPosition( 0, 1 );
      }

   float tmp;

   if( mRecordMeter && mWhichMeters & kWithRecordMeter ) {
      //auto inButton = static_cast<AButton*>(FindWindow(ID_INPUT_BUTTON));

      mRecordMeter->SetMinSize( wxSize( std::max(width-wd,20), std::max(height-hd,20)));
      mInSizer->SetItemPosition( mRecordMeter, pos );

   }

   if( mPlaydMeter && mWhichMeters & kWithPlayMeter ) {

      mPlayMeter->SetMinSize( wxSize( std::max(width-wd,20), std::max(height-hd,20)));
      mOutSizer->SetItemPosition( mPlayMeter, pos );
   }

   // And make it happen
   Layout();
   Fit();
}

bool MeterToolBar::Expose( bool show )
{
   auto &projectAudioIO = ProjectAudioIO::Get( mProject );
   if( show ) {
      if( mPlayMeter ) {
         projectAudioIO.SetPlaybackMeter( mPlayMeter->GetMeter() );
      }

      if( mRecordMeter ) {
         projectAudioIO.SetCaptureMeter( mRecordMeter->GetMeter() );
      }
   } else {
      if( mPlayMeter &&
         projectAudioIO.GetPlaybackMeter() == mPlayMeter->GetMeter() ) {
         projectAudioIO.SetPlaybackMeter( nullptr );
      }

      if( mRecordMeter &&
         projectAudioIO.GetCaptureMeter() == mRecordMeter->GetMeter() ) {
         projectAudioIO.SetCaptureMeter( nullptr );
      }
   }

   return ToolBar::Expose( show );
}

// The meter's sizing code does not take account of the resizer
// Hence after docking we need to enlarge the bar (using fit)
// so that the resizer can be reached.
void MeterToolBar::SetDocked(ToolDock *dock, bool pushed) {
   ToolBar::SetDocked(dock, pushed);
   Fit();
}

static RegisteredToolbarFactory factory1{ RecordMeterBarID,
   []( TenacityProject &project ){
      return ToolBar::Holder{
         safenew MeterToolBar{ project, RecordMeterBarID } }; }
};
static RegisteredToolbarFactory factory2{ PlayMeterBarID,
   []( TenacityProject &project ){
      return ToolBar::Holder{
         safenew MeterToolBar{ project, PlayMeterBarID } }; }
};

void MeterToolBar::OnInputButton(wxCommandEvent &event)
{
   auto Button = static_cast<AButton*>(FindWindow(ID_INPUT_BUTTON));
   auto pos=Button->GetPosition();
   pos.y+=Button->GetMinHeight();

   ShowMenu(true, pos);
   Button->InteractionOver();

}
void MeterToolBar::OnOutputButton(wxCommandEvent &event)
{
   auto Button = static_cast<AButton*>(FindWindow(ID_OUTPUT_BUTTON));
   auto pos=Button->GetPosition();
   pos.y+=Button->GetMinHeight();

   ShowMenu(false, pos);
   Button->InteractionOver();
}

void MeterToolBar::OnHost(wxCommandEvent &) {
   //workaround to set prefs dialog to devices
   gPrefs->Write(wxT("/Prefs/PrefsCategory"), 0);
   gPrefs->Flush();

   GlobalPrefsDialog dialog(&GetProjectPanel( mProject ) , &mProject );

// Doesn't work?
//   dialog.SelectPageByName(XO("Devices").Translation());

   dialog.ShowModal();
}

void MeterToolBar::BuildMenus (bool InputMenu){
   auto gAudioIO = AudioIOBase::Get();
   bool mMonitoring= gAudioIO->IsMonitoring();
   bool mActive= false;

   int i=0;

   bool audioStreamActive=false;
   auto host = AudioIOHost.Read();

   if (gAudioIO) {
      audioStreamActive = gAudioIO->IsStreamActive() && !mMonitoring;
      }
   if  (!audioStreamActive) {
      mIOMenu->Append(OnHostID, host+"...");
      mIOMenu->AppendSeparator();
   }

   if (InputMenu) {
      const std::vector<Device> &inMaps  = DeviceManager::Instance()->GetInputDevices();
      if  (!audioStreamActive) {

         auto devUseName = AudioIORecordingDevice.Read();
         for (auto & device : inMaps) {
            if (host == device.GetHostName())
            {
               bool check = (device.GetName() == devUseName);

               mIOMenu->AppendCheckItem(OnDeviceID+i,device.GetName())->Check(check);
               Bind(wxEVT_COMMAND_MENU_SELECTED, MeterToolBar::SetInDevice, (OnDeviceID+i));
               i++;
            }
         }

         auto device = AudioIORecordingDevice.Read();

         long oldChannels = AudioIORecordChannels.Read();
         for (auto & dev: inMaps) {
            if (device == dev.GetName() && host == dev.GetHostName())
            {
               // add one selection for each channel of this source
               for (size_t j = 0; j < (unsigned int) dev.GetNumChannels(); j++) {
                  wxString name;

                  if (j == 0) {
                     name = _("1 (Mono)");
                  }
                  else if (j == 1) {
                     name = _("2 (Stereo)");
                  }
                  else {
                     name = wxString::Format(wxT("%d"), (int) j + 1);
                  }
                     mChannelsMenu->AppendCheckItem(j+1000, name)->Check(oldChannels==j+1);
                     Bind(wxEVT_COMMAND_MENU_SELECTED, MeterToolBar::SetChannelCount, j+1000);
               }
               break;
            }
         }
         mIOMenu->AppendSubMenu(mChannelsMenu, _("Channels"));
         mIOMenu->AppendSeparator();
      }
      mIOMenu->Append(OnRecPreferencesID, _("Meter Options..."));
      wxMenuItem *mi;
      if (mMonitoring)
         mi = mIOMenu->Append(OnMonitorID, _("Stop Monitoring"));
      else
         mi = mIOMenu->Append(OnMonitorID, _("Start Monitoring"));
      mi->Enable(!mActive || mMonitoring);

   } else {
      const std::vector<Device> &outMaps = DeviceManager::Instance()->GetOutputDevices();
      if (!audioStreamActive) {
         auto devUseName = AudioIOPlaybackDevice.Read();
         for (auto & device : outMaps) {
            if (host == device.GetHostName()) {
               bool check = (device.GetName() == devUseName);

               mIOMenu->AppendCheckItem(OnDeviceID+i,device.GetName())->Check(check);
               Bind(wxEVT_COMMAND_MENU_SELECTED, MeterToolBar::SetOutDevice, (OnDeviceID+i));
               i++;
            }
         }
         mIOMenu->AppendSeparator();
       }
      mIOMenu->Append(OnPlayPreferencesID, _("Meter Options..."));
   }

}

void MeterToolBar::ClearMenus() {
   while ( mIOMenu->GetMenuItemCount() > 0 )
   {
      mIOMenu->Delete( *( mIOMenu->GetMenuItems().begin() ) );
   }
   while ( mChannelsMenu->GetMenuItemCount() > 0 )
   {
      mChannelsMenu->Delete( *( mChannelsMenu->GetMenuItems().begin() ) );
   }
}

void MeterToolBar::ShowMenu(bool InputMenu, wxPoint pos)
{
   BuildMenus(InputMenu);
   PopupMenu(mIOMenu, pos);

#if wxUSE_ACCESSIBILITY
   GetAccessible()->NotifyEvent(wxACC_EVENT_OBJECT_FOCUS,
                                this,
                                wxOBJID_CLIENT,
                                wxACC_SELF);
#endif
   ClearMenus();
}

void MeterToolBar::SetChannelCount(wxCommandEvent &event){
int id=event.GetId()-999;
   AudioIORecordChannels.Write(id);
}

void MeterToolBar::UpdateChannelCount() {
   const std::vector<Device> &inMaps = DeviceManager::Instance()->GetInputDevices();
   auto host = AudioIOHost.Read();
   auto device = AudioIORecordingDevice.Read();
   long newChannels;

   auto oldChannels = AudioIORecordChannels.Read();
   for (auto & dev: inMaps) {
      if (device == dev.GetName() &&
          host   == dev.GetHostName()) {

         newChannels = dev.GetNumChannels();
         if (oldChannels <= newChannels && oldChannels >= 1) {
            newChannels = oldChannels;
         }
         AudioIORecordChannels.Write(newChannels);
         break;
      }
   }
}

void MeterToolBar::SetOutDevice(wxCommandEvent &event){

   const std::vector<Device> &outMaps = DeviceManager::Instance()->GetOutputDevices();
   auto host = AudioIOHost.Read();
   int i=0; //HostIndex doesnt match array index in all versions of PortAudio. Count manually
   for (auto & dev: outMaps) {
      if ( host == dev.GetHostName() ) {
         int id= i + event.GetId() - OnDeviceID;
         auto &device = outMaps[id];
         AudioIOPlaybackDevice.Write(device.GetName());
      }
      i++;
   }
   gPrefs->Flush();

}

void MeterToolBar::SetInDevice(wxCommandEvent &event){

   const std::vector<Device> &inMaps = DeviceManager::Instance()->GetInputDevices();
   auto host = AudioIOHost.Read();
   int i=0; //HostIndex doesnt match array index in all versions of PortAudio. Count manually
   for (auto & dev: inMaps) {
      if ( host == dev.GetHostName() ) {
          int id = i + event.GetId() - OnDeviceID;
         auto &device = inMaps[id];
         AudioIORecordingDevice.Write(device.GetName());
         break;
      }
      i++;
   }
   UpdateChannelCount();
   gPrefs->Flush();

}


void MeterToolBar::OnMonitor(wxCommandEvent &){
   mRecordMeter->StartMonitoring();
}

void MeterToolBar::OnRecPreferences(wxCommandEvent &event){
   mRecordMeter->ShowPreferences();
}
void MeterToolBar::OnPlayPreferences(wxCommandEvent &event){
   mPlayMeter->ShowPreferences();
}


#include "ToolManager.h"

namespace {
AttachedToolBarMenuItem sAttachment1{
   /* i18n-hint: Clicking this menu item shows the toolbar
      with the recording level meters */
   RecordMeterBarID, wxT("ShowRecordMeterTB"), XXO("&Recording Meter Toolbar"),
   {}, {}
};
AttachedToolBarMenuItem sAttachment2{
   /* i18n-hint: Clicking this menu item shows the toolbar
      with the playback level meter */
   PlayMeterBarID, wxT("ShowPlayMeterTB"), XXO("&Playback Meter Toolbar"),
   {}, {}
};
}
