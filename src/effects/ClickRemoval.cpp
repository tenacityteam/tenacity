/**********************************************************************

  Audacity: A Digital Audio Editor

  ClickRemoval.cpp

  Craig DeForest

*******************************************************************//**

\class EffectClickRemoval
\brief An Effect for removing clicks.

  Clicks are identified as small regions of high amplitude compared
  to the surrounding chunk of sound.  Anything sufficiently tall compared
  to a large (2048 sample) window around it, and sufficiently narrow,
  is considered to be a click.

  The structure was largely stolen from Domonic Mazzoni's NoiseRemoval
  module, and reworked for the NEW effect.

  This file is intended to become part of Audacity.  You may modify
  and/or distribute it under the same terms as Audacity itself.

*//*******************************************************************/


#include "ClickRemoval.h"
#include "LoadEffects.h"

#include <cmath>

#include <wx/intl.h>
#include <wx/slider.h>
#include <wx/valgen.h>

// Tenacity libraries
#include <lib-preferences/Prefs.h>

#include "../shuttle/Shuttle.h"
#include "../shuttle/ShuttleGui.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/valnum.h"

#include "../WaveTrack.h"

enum
{
   ID_Thresh = 10000,
   ID_Width
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def      Min      Max      Scale
Param( Threshold, int,     wxT("Threshold"),  200,     0,       900,     1  );
Param( Width,     int,     wxT("Width"),      20,      0,       40,      1  );

const ComponentInterfaceSymbol EffectClickRemoval::Symbol
{ XO("Click Removal") };

namespace{ BuiltinEffectsModule::Registration< EffectClickRemoval > reg; }

BEGIN_EVENT_TABLE(EffectClickRemoval, wxEvtHandler)
    EVT_SLIDER(ID_Thresh, EffectClickRemoval::OnThreshSlider)
    EVT_SLIDER(ID_Width, EffectClickRemoval::OnWidthSlider)
    EVT_TEXT(ID_Thresh, EffectClickRemoval::OnThreshText)
    EVT_TEXT(ID_Width, EffectClickRemoval::OnWidthText)
END_EVENT_TABLE()

EffectClickRemoval::EffectClickRemoval()
{
   mThresholdLevel = DEF_Threshold;
   mClickWidth = DEF_Width;

   SetLinearEffectFlag(false);
}

EffectClickRemoval::~EffectClickRemoval()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectClickRemoval::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectClickRemoval::GetDescription()
{
   return XO("Click Removal is designed to remove clicks on audio tracks");
}

ManualPageID EffectClickRemoval::ManualPage()
{
   return L"Click_Removal";
}

// EffectDefinitionInterface implementation

EffectType EffectClickRemoval::GetType()
{
   return EffectTypeProcess;
}

// EffectClientInterface implementation
bool EffectClickRemoval::DefineParams( ShuttleParams & S ){
   S.SHUTTLE_PARAM( mThresholdLevel, Threshold );
   S.SHUTTLE_PARAM( mClickWidth, Width );
   return true;
}

bool EffectClickRemoval::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(KEY_Threshold, mThresholdLevel);
   parms.Write(KEY_Width, mClickWidth);

   return true;
}

bool EffectClickRemoval::SetAutomationParameters(CommandParameters & parms)
{
   ReadAndVerifyInt(Threshold);
   ReadAndVerifyInt(Width);

   mThresholdLevel = Threshold;
   mClickWidth = Width;

   return true;
}

// Effect implementation

bool EffectClickRemoval::CheckWhetherSkipEffect()
{
   return ((mClickWidth == 0) || (mThresholdLevel == 0));
}

bool EffectClickRemoval::Startup()
{
   wxString base = wxT("/Effects/ClickRemoval/");

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + wxT("Migrated")))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      mThresholdLevel = gPrefs->Read(base + wxT("ClickThresholdLevel"), 200);
      if ((mThresholdLevel < MIN_Threshold) || (mThresholdLevel > MAX_Threshold))
      {  // corrupted Prefs?
         mThresholdLevel = 0;  //Off-skip
      }
      mClickWidth = gPrefs->Read(base + wxT("ClickWidth"), 20);
      if ((mClickWidth < MIN_Width) || (mClickWidth > MAX_Width))
      {  // corrupted Prefs?
         mClickWidth = 0;  //Off-skip
      }

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + wxT("Migrated"), true);
      gPrefs->Flush();
   }

   return true;
}

bool EffectClickRemoval::Process()
{
   this->CopyInputTracks(); // Set up mOutputTracks.
   bool bGoodResult = true;
   mbDidSomething = false;

   int count = 0;
   for( auto track : mOutputTracks->Selected< WaveTrack >() ) {
      double trackStart = track->GetStartTime();
      double trackEnd = track->GetEndTime();
      double t0 = mT0 < trackStart? trackStart: mT0;
      double t1 = mT1 > trackEnd? trackEnd: mT1;

      if (t1 > t0) {
         auto start = track->TimeToLongSamples(t0);
         auto end = track->TimeToLongSamples(t1);
         auto len = end - start;

         if (!ProcessOne(count, track, start, len))
         {
            bGoodResult = false;
            break;
         }
      }

      count++;
   }
   if (bGoodResult && !mbDidSomething) // Processing successful, but ineffective.
      Effect::MessageBox(
         XO("Algorithm not effective on this audio. Nothing changed."),
         wxOK | wxICON_ERROR );

   this->ReplaceProcessedTracks(bGoodResult && mbDidSomething);
   return bGoodResult && mbDidSomething;
}

bool EffectClickRemoval::ProcessOne(int count, WaveTrack * track, sampleCount start, sampleCount len)
{
   if (len <= windowSize / 2)
   {
      Effect::MessageBox(
         XO("Selection must be larger than %d samples.")
            .Format(windowSize / 2),
         wxOK | wxICON_ERROR );
      return false;
   }

   auto idealBlockLen = track->GetMaxBlockSize() * 4;
   if (idealBlockLen % windowSize != 0)
      idealBlockLen += (windowSize - (idealBlockLen % windowSize));

   bool bResult = true;
   decltype(len) s = 0;
   Floats buffer{ idealBlockLen };
   Floats datawindow{ windowSize };
   while ((len - s) > windowSize / 2)
   {
      auto block = limitSampleBufferSize( idealBlockLen, len - s );

      track->GetFloats(buffer.get(), start + s, block);

      for (decltype(block) i = 0; i + windowSize / 2 < block; i += windowSize / 2)
      {
         auto wcopy = std::min( windowSize, block - i );

         for(decltype(wcopy) j = 0; j < wcopy; j++)
            datawindow[j] = buffer[i+j];
         for(auto j = wcopy; j < windowSize; j++)
            datawindow[j] = 0;

         mbDidSomething |= RemoveClicks(datawindow);

         for(decltype(wcopy) j = 0; j < wcopy; j++)
           buffer[i+j] = datawindow[j];
      }

      if (mbDidSomething) // RemoveClicks() actually did something.
         track->Set((samplePtr) buffer.get(), floatSample, start + s, block);

      s += block;

      if (TrackProgress(count, s.as_double() /
                               len.as_double())) {
         bResult = false;
         break;
      }
   }

   return bResult;
}

bool EffectClickRemoval::RemoveClicks(Floats & buffer) const
{
   bool bResult = false; // This effect usually does nothing.
   size_t i;
   size_t j;
   size_t clickStart = SIZE_MAX;

   float msw;
   size_t ww;
   size_t s2 = windowSize/4;
   Floats ms_seq{ windowSize };
   Floats b2{ windowSize };

   for( i=0; i<windowSize; i++)
      ms_seq[i] = b2[i] = buffer[i]*buffer[i];

   /* Shortcut for rms - multiple passes through b2, accumulating
    * as we go.
    */
   for(i=1; i <windowSize/2; i *= 2) {
      for(j=0;j<windowSize-i; j++)
         ms_seq[j] += ms_seq[j+i];
   }

   for( i=0; i<windowSize/2; i++ ) {
      ms_seq[i] /= windowSize/2;
   }
   /* ww runs from about 4 to mClickWidth.  wrc is the reciprocal;
    * chosen so that integer roundoff doesn't clobber us.
    */
   size_t wrc;
   for(wrc=(size_t)mClickWidth/4; wrc>=1; wrc /= 2) {
      ww = (size_t)mClickWidth/wrc;

      for( i=0; i<windowSize/2; i++ ){
         msw = 0;
         for( j=0; j<ww; j++) {
            msw += b2[i+s2+j];
         }
         msw /= ww;

         if(msw >= mThresholdLevel * ms_seq[i]/10) {
            if( clickStart == SIZE_MAX ) {
               clickStart = i;
            }
         } else if(clickStart != SIZE_MAX) {
            if((i-clickStart) <= ww*2) {
               bResult = true;
               float lv = buffer[clickStart+s2];
               float rv = buffer[i+ww+s2];
               for(j=clickStart+s2; j<i+ww+s2; j++) {
                  buffer[j]= (rv*(j-(clickStart+s2)) + lv*(i+ww+s2-j))/(float)(i+ww-clickStart);
                  b2[j] = buffer[j]*buffer[j];
               }
            }
            clickStart = SIZE_MAX;
         }
      }
   }
   return bResult;
}

void EffectClickRemoval::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);
   S.SetBorder(10);

   S.StartMultiColumn(3, wxEXPAND);
   S.SetStretchyCol(2);
   {
      // Threshold
      mThreshT = S.Id(ID_Thresh)
         .Validator<IntegerValidator<int>>(
            &mThresholdLevel, NumValidatorStyle::DEFAULT,
            MIN_Threshold, MAX_Threshold
         )
         .AddTextBox(XXO("&Threshold (lower is more sensitive):"),
                     wxT(""),
                     10);

      mThreshS = S.Id(ID_Thresh)
         .Name(XO("Threshold"))
         .Style(wxSL_HORIZONTAL)
         .Validator<wxGenericValidator>(&mThresholdLevel)
         .MinSize( { 150, -1 } )
         .AddSlider( {}, mThresholdLevel, MAX_Threshold, MIN_Threshold);

      // Click width
      mWidthT = S.Id(ID_Width)
         .Validator<IntegerValidator<int>>(
            &mClickWidth, NumValidatorStyle::DEFAULT, MIN_Width, MAX_Width)
         .AddTextBox(XXO("Max &Spike Width (higher is more sensitive):"),
                     wxT(""),
                     10);

      mWidthS = S.Id(ID_Width)
         .Name(XO("Max Spike Width"))
         .Style(wxSL_HORIZONTAL)
         .Validator<wxGenericValidator>(&mClickWidth)
         .MinSize( { 150, -1 } )
         .AddSlider( {}, mClickWidth, MAX_Width, MIN_Width);
   }
   S.EndMultiColumn();

   return;
}

bool EffectClickRemoval::TransferDataToWindow()
{
   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   return true;
}

bool EffectClickRemoval::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   return true;
}

void EffectClickRemoval::OnWidthText(wxCommandEvent & WXUNUSED(evt))
{
   mWidthT->GetValidator()->TransferFromWindow();
   mWidthS->GetValidator()->TransferToWindow();
}

void EffectClickRemoval::OnThreshText(wxCommandEvent & WXUNUSED(evt))
{
   mThreshT->GetValidator()->TransferFromWindow();
   mThreshS->GetValidator()->TransferToWindow();
}

void EffectClickRemoval::OnWidthSlider(wxCommandEvent & WXUNUSED(evt))
{
   mWidthS->GetValidator()->TransferFromWindow();
   mWidthT->GetValidator()->TransferToWindow();
}

void EffectClickRemoval::OnThreshSlider(wxCommandEvent & WXUNUSED(evt))
{
   mThreshS->GetValidator()->TransferFromWindow();
   mThreshT->GetValidator()->TransferToWindow();
}
