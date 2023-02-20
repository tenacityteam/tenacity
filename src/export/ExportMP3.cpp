/**********************************************************************

  Tenacity: A Digital Audio Editor

  ExportMP3.cpp

  Joshua Haberman

  Historically, Audacity required LAME to be loaded at runtime.
  Starting in 2017, all patents surrounding the MP3 format have expired,
  making it possible to link LAME directly against Audacity without
  legal issues.

  For historical reasons, we do the following when exporting to MP3:

      1. Initialize the library.
      2. Set, at minimum, the following global options:
          i.  input sample rate
          ii. input channels
      3. Encode the stream
      4. Call the finishing routine

***********************************************************************

  Copyright 2002, 2003 Joshua Haberman.
  Some portions may be Copyright 2003 Paolo Patruno.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*******************************************************************//**

\class MP3Exporter
\brief Class used to export MP3 files

*//********************************************************************/


#include "ExportMP3.h"
#include "ExportPlugin.h"

#include <wx/app.h>
#include <wx/defs.h>

#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/dynlib.h>
#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/mimetype.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/window.h>

// Tenacity libraries
#include <lib-files/FileNames.h>
#include <lib-files/wxFileNameWrapper.h>
#include <lib-math/float_cast.h>
#include <lib-preferences/Prefs.h>
#include <lib-project-rate/ProjectRate.h>

#include "../Mix.h"
#include "../ProjectSettings.h"
#include "../ProjectWindow.h"
#include "../SelectFile.h"
#include "../shuttle/ShuttleGui.h"
#include "../Tags.h"
#include "../Track.h"
#include "../widgets/HelpSystem.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/ProgressDialog.h"

#include "Export.h"

#include <lame/lame.h>

#ifdef USE_LIBID3TAG
#include <id3tag.h>
#endif

//----------------------------------------------------------------------------
// ExportMP3Options
//----------------------------------------------------------------------------

enum MP3ChannelMode : unsigned {
   CHANNEL_JOINT = 0,
   CHANNEL_STEREO = 1,
   CHANNEL_MONO = 2,
};

enum : int {
   QUALITY_2 = 2,

   //ROUTINE_FAST = 0,
   //ROUTINE_STANDARD = 1,

   PRESET_INSANE = 0,
   PRESET_EXTREME = 1,
   PRESET_STANDARD = 2,
   PRESET_MEDIUM = 3,
};

/* i18n-hint: kbps is the bitrate of the MP3 file, kilobits per second*/
inline TranslatableString n_kbps( int n ){ return XO("%d kbps").Format( n ); }

static const TranslatableStrings fixRateNames {
   n_kbps(320),
   n_kbps(256),
   n_kbps(224),
   n_kbps(192),
   n_kbps(160),
   n_kbps(144),
   n_kbps(128),
   n_kbps(112),
   n_kbps(96),
   n_kbps(80),
   n_kbps(64),
   n_kbps(56),
   n_kbps(48),
   n_kbps(40),
   n_kbps(32),
   n_kbps(24),
   n_kbps(16),
   n_kbps(8),
};

static const std::vector<int> fixRateValues {
   320,
   256,
   224,
   192,
   160,
   144,
   128,
   112,
   96,
   80,
   64,
   56,
   48,
   40,
   32,
   24,
   16,
   8,
};

static const TranslatableStrings varRateNames {
   XO("220-260 kbps (Best Quality)"),
   XO("200-250 kbps"),
   XO("170-210 kbps"),
   XO("155-195 kbps"),
   XO("145-185 kbps"),
   XO("110-150 kbps"),
   XO("95-135 kbps"),
   XO("80-120 kbps"),
   XO("65-105 kbps"),
   XO("45-85 kbps (Smaller files)"),
};
/*
static const TranslatableStrings varModeNames {
   XO("Fast"),
   XO("Standard"),
};
*/
static const TranslatableStrings setRateNames {
   /* i18n-hint: Slightly humorous - as in use an insane precision with MP3.*/
   XO("Insane, 320 kbps"),
   XO("Extreme, 220-260 kbps"),
   XO("Standard, 170-210 kbps"),
   XO("Medium, 145-185 kbps"),
};

static const TranslatableStrings setRateNamesShort {
   /* i18n-hint: Slightly humorous - as in use an insane precision with MP3.*/
   XO("Insane"),
   XO("Extreme"),
   XO("Standard"),
   XO("Medium"),
};

static const std::vector< int > sampRates {
   8000,
   11025,
   12000,
   16000,
   22050,
   24000,
   32000,
   44100,
   48000,
};

#define ID_SET 7000
#define ID_VBR 7001
#define ID_ABR 7002
#define ID_CBR 7003
#define ID_QUALITY 7004
#define ID_MONO 7005

class ExportMP3Options final : public wxPanelWrapper
{
public:

   ExportMP3Options(wxWindow *parent, int format);
   virtual ~ExportMP3Options();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

   void OnSET(wxCommandEvent& evt);
   void OnVBR(wxCommandEvent& evt);
   void OnABR(wxCommandEvent& evt);
   void OnCBR(wxCommandEvent& evt);
   void OnQuality(wxCommandEvent& evt);
   void OnMono(wxCommandEvent& evt);

   void LoadNames(const TranslatableStrings &choices);

private:

   wxRadioButton *mStereo;
   wxRadioButton *mJoint;
   wxCheckBox    *mMono;
   wxRadioButton *mSET;
   wxRadioButton *mVBR;
   wxRadioButton *mABR;
   wxRadioButton *mCBR;
   wxChoice *mRate;
   //wxChoice *mMode;

   long mSetRate;
   long mVbrRate;
   long mAbrRate;
   long mCbrRate;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ExportMP3Options, wxPanelWrapper)
   EVT_RADIOBUTTON(ID_SET,    ExportMP3Options::OnSET)
   EVT_RADIOBUTTON(ID_VBR,    ExportMP3Options::OnVBR)
   EVT_RADIOBUTTON(ID_ABR,    ExportMP3Options::OnABR)
   EVT_RADIOBUTTON(ID_CBR,    ExportMP3Options::OnCBR)
   EVT_CHOICE(wxID_ANY,       ExportMP3Options::OnQuality)
   EVT_CHECKBOX(ID_MONO,      ExportMP3Options::OnMono)
END_EVENT_TABLE()

///
///
ExportMP3Options::ExportMP3Options(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   mSetRate = gPrefs->Read(wxT("/FileFormats/MP3SetRate"), PRESET_STANDARD);
   mVbrRate = gPrefs->Read(wxT("/FileFormats/MP3VbrRate"), QUALITY_2);
   mAbrRate = gPrefs->Read(wxT("/FileFormats/MP3AbrRate"), 192);
   mCbrRate = gPrefs->Read(wxT("/FileFormats/MP3CbrRate"), 192);

   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportMP3Options::~ExportMP3Options()
{
   TransferDataFromWindow();
}

EnumSetting< MP3RateMode > MP3RateModeSetting{
   wxT("/FileFormats/MP3RateModeChoice"),
   {
      { wxT("SET"), XXO("Preset") },
      { wxT("VBR"), XXO("Variable") },
      { wxT("ABR"), XXO("Average") },
      { wxT("CBR"), XXO("Constant") },
   },
   0, // MODE_SET

   // for migrating old preferences:
   {
      MODE_SET, MODE_VBR, MODE_ABR, MODE_CBR
   },
   wxT("/FileFormats/MP3RateMode"),
};

static EnumSetting< MP3ChannelMode > MP3ChannelModeSetting{
   wxT("/FileFormats/MP3ChannelModeChoice"),
   {
      EnumValueSymbol{ wxT("JOINT"), XXO("Joint Stereo") },
      EnumValueSymbol{ wxT("STEREO"), XXO("Stereo") },
   },
   0, // CHANNEL_JOINT

   // for migrating old preferences:
   {
      CHANNEL_JOINT, CHANNEL_STEREO,
   },
   wxT("/FileFormats/MP3ChannelMode"),
};

///
///
void ExportMP3Options::PopulateOrExchange(ShuttleGui & S)
{
   bool mono = false;
   gPrefs->Read(wxT("/FileFormats/MP3ForceMono"), &mono, 0);

   const TranslatableStrings *choices = nullptr;
   const std::vector< int > *codes = nullptr;
   //bool enable;
   int defrate;

   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.SetStretchyCol(1);
            S.StartTwoColumn();
            {
               S.AddPrompt(XXO("Bit Rate Mode:"));

               // Bug 2692: Place button group in panel so tabbing will work and,
               // on the Mac, VoiceOver will announce as radio buttons.
               S.StartPanel();
               {
                  S.StartHorizontalLay();
                  {
                     S.StartRadioButtonGroup(MP3RateModeSetting);
                     {
                        mSET = S.Id(ID_SET).TieRadioButton();
                        mVBR = S.Id(ID_VBR).TieRadioButton();
                        mABR = S.Id(ID_ABR).TieRadioButton();
                        mCBR = S.Id(ID_CBR).TieRadioButton();
                     }
                     S.EndRadioButtonGroup();
                  }
                  S.EndHorizontalLay();
               }
               S.EndPanel();

               /* PRL: unfortunately this bit of procedural code must
                interrupt the mostly-declarative dialog description, until
                we find a better solution.  Because when shuttling values
                from the dialog, we must shuttle out the MP3RateModeSetting
                first. */

               switch( MP3RateModeSetting.ReadEnum() ) {
                  case MODE_SET:
                     choices = &setRateNames;
                     //enable = true;
                     defrate = mSetRate;
                     break;

                  case MODE_VBR:
                     choices = &varRateNames;
                     //enable = true;
                     defrate = mVbrRate;
                     break;

                  case MODE_ABR:
                     choices = &fixRateNames;
                     codes = &fixRateValues;
                     //enable = false;
                     defrate = mAbrRate;
                     break;

                  case MODE_CBR:
                  default:
                     choices = &fixRateNames;
                     codes = &fixRateValues;
                     //enable = false;
                     defrate = mCbrRate;
                     break;
               }

               mRate = S.Id(ID_QUALITY).TieNumberAsChoice(
                  XXO("Quality"),
                  { wxT("/FileFormats/MP3Bitrate"), defrate },
                  *choices,
                  codes
               );
               /*
               mMode = S.Disable(!enable)
                  .TieNumberAsChoice(
                     XXO("Variable Speed:"),
                     { wxT("/FileFormats/MP3VarMode"), ROUTINE_FAST },
                     varModeNames );
               */
               S.AddPrompt(XXO("Channel Mode:"));
               S.StartMultiColumn(2, wxEXPAND);
               {
                  // Bug 2692: Place button group in panel so tabbing will work and,
                  // on the Mac, VoiceOver will announce as radio buttons.
                  S.StartPanel();
                  {
                     S.StartHorizontalLay();
                     {
                        S.StartRadioButtonGroup(MP3ChannelModeSetting);
                        {
                           mJoint = S.Disable(mono)
                              .TieRadioButton();
                           mStereo = S.Disable(mono)
                              .TieRadioButton();
                        }
                        S.EndRadioButtonGroup();
                     }
                     S.EndHorizontalLay();
                  }
                  S.EndPanel();

                  mMono = S.Id(ID_MONO).AddCheckBox(XXO("Force export to mono"), mono);
               }
               S.EndMultiColumn();
            }
            S.EndTwoColumn();
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportMP3Options::TransferDataToWindow()
{
   return true;
}

bool ExportMP3Options::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Write(wxT("/FileFormats/MP3SetRate"), mSetRate);
   gPrefs->Write(wxT("/FileFormats/MP3VbrRate"), mVbrRate);
   gPrefs->Write(wxT("/FileFormats/MP3AbrRate"), mAbrRate);
   gPrefs->Write(wxT("/FileFormats/MP3CbrRate"), mCbrRate);
   gPrefs->Flush();

   return true;
}

namespace {

int ValidateValue( int nValues, int value, int defaultValue )
{
   return (value >= 0 && value < nValues) ? value : defaultValue;
}

int ValidateValue( const std::vector<int> &values, int value, int defaultValue )
{
   auto start = values.begin(), finish = values.end(),
      iter = std::find( start, finish, value );
   return ( iter != finish ) ? value : defaultValue;
}

int ValidateIndex( const std::vector<int> &values, int value, int defaultIndex )
{
   auto start = values.begin(), finish = values.end(),
      iter = std::find( start, finish, value );
   return ( iter != finish ) ? static_cast<int>( iter - start ) : defaultIndex;
}

}

///
///
void ExportMP3Options::OnSET(wxCommandEvent& WXUNUSED(event))
{
   LoadNames(setRateNames);

   mRate->SetSelection(ValidateValue(setRateNames.size(), mSetRate, 2));
   mRate->Refresh();
   //mMode->Enable(true);
}

///
///
void ExportMP3Options::OnVBR(wxCommandEvent& WXUNUSED(event))
{
   LoadNames(varRateNames);

   mRate->SetSelection(ValidateValue(varRateNames.size(), mVbrRate, 2));
   mRate->Refresh();
   //mMode->Enable(true);
}

///
///
void ExportMP3Options::OnABR(wxCommandEvent& WXUNUSED(event))
{
   LoadNames(fixRateNames);

   mRate->SetSelection(ValidateIndex(fixRateValues, mAbrRate, 10));
   mRate->Refresh();
   //mMode->Enable(false);
}

///
///
void ExportMP3Options::OnCBR(wxCommandEvent& WXUNUSED(event))
{
   LoadNames(fixRateNames);

   mRate->SetSelection(ValidateIndex(fixRateValues, mCbrRate, 10));
   mRate->Refresh();
   //mMode->Enable(false);
}

void ExportMP3Options::OnQuality(wxCommandEvent& WXUNUSED(event))
{
   int sel = mRate->GetSelection();

   if (mSET->GetValue()) {
      mSetRate = sel;
   }
   else if (mVBR->GetValue()) {
      mVbrRate = sel;
   }
   else if (mABR->GetValue()) {
      mAbrRate = fixRateValues[ sel ];
   }
   else {
      mCbrRate = fixRateValues[ sel ];
   }
}

void ExportMP3Options::OnMono(wxCommandEvent& /*evt*/)
{
   bool mono = false;
   mono = mMono->GetValue();
   mJoint->Enable(!mono);
   mStereo->Enable(!mono);

   gPrefs->Write(wxT("/FileFormats/MP3ForceMono"), mono);
   gPrefs->Flush();
}

void ExportMP3Options::LoadNames(const TranslatableStrings &names)
{
   mRate->Clear();
   for (const auto &name : names)
      mRate->Append( name.Translation() );
}

//----------------------------------------------------------------------------
// MP3Exporter
//----------------------------------------------------------------------------

class MP3Exporter
{
public:
   enum AskUser
   {
      No,
      Maybe,
      Yes
   };

   MP3Exporter();
   ~MP3Exporter();

   /* These global settings keep state over the life of the object */
   void SetMode(int mode);
   void SetBitrate(int rate);
   void SetQuality(int q/*, int r*/);
   void SetChannel(int mode);

   /* Virtual methods that must be supplied by library interfaces */

   /* initialize the library interface */
   bool InitLibrary();
   void FreeLibrary();

   /* returns the number of samples PER CHANNEL to send for each call to EncodeBuffer */
   int InitializeStream(unsigned channels, int sampleRate);

   /* In bytes. must be called AFTER InitializeStream */
   int GetOutBufferSize();

   /* returns the number of bytes written. input is interleaved if stereo*/
   int EncodeBuffer(float inbuffer[], unsigned char outbuffer[]);
   int EncodeRemainder(float inbuffer[], int nSamples,
                       unsigned char outbuffer[]);

   int EncodeBufferMono(float inbuffer[], unsigned char outbuffer[]);
   int EncodeRemainderMono(float inbuffer[], int nSamples,
                           unsigned char outbuffer[]);

   int FinishStream(unsigned char outbuffer[]);
   void CancelEncoding();

   bool PutInfoTag(wxFFile & f, wxFileOffset off);

private:
   bool mLibIsExternal;

   bool mEncoding;
   int mMode;
   int mBitrate;
   int mQuality;
   //int mRoutine;
   int mChannel;

   lame_global_flags *mGlobalFlags;

   static const int mSamplesPerChunk = 220500;
   // See lame.h/lame_encode_buffer() for further explanation
   // As coded here, this should be the worst case.
   static const int mOutBufferSize =
      mSamplesPerChunk * (320 / 8) / 8 + 4 * 1152 * (320 / 8) / 8 + 512;

   // See MAXFRAMESIZE in libmp3lame/VbrTag.c for explanation of 2880.
   unsigned char mInfoTagBuf[2880];
   size_t mInfoTagLen;
};

MP3Exporter::MP3Exporter()
{
   mEncoding = false;
   mGlobalFlags = nullptr;

   mBitrate = 128;
   mQuality = QUALITY_2;
   mChannel = CHANNEL_STEREO;
   mMode = MODE_CBR;
   //mRoutine = ROUTINE_FAST;

   InitLibrary();
}

MP3Exporter::~MP3Exporter()
{
   FreeLibrary();
}

void MP3Exporter::SetMode(int mode)
{
   mMode = mode;
}

void MP3Exporter::SetBitrate(int rate)
{
   mBitrate = rate;
}

void MP3Exporter::SetQuality(int q/*, int r*/)
{
   mQuality = q;
   //mRoutine = r;
}

void MP3Exporter::SetChannel(int mode)
{
   mChannel = mode;
}

bool MP3Exporter::InitLibrary()
{
   mGlobalFlags = lame_init();

   if (!mGlobalFlags)
   {
      return false;
   }

   return true;
}

void MP3Exporter::FreeLibrary()
{
   if (mGlobalFlags)
   {
      lame_close(mGlobalFlags);
      mGlobalFlags = nullptr;
   }
}

int MP3Exporter::InitializeStream(unsigned channels, int sampleRate)
{
   if (channels > 2) {
      return -1;
   }

   lame_set_error_protection(mGlobalFlags, false);
   lame_set_num_channels(mGlobalFlags, channels);
   lame_set_in_samplerate(mGlobalFlags, sampleRate);
   lame_set_out_samplerate(mGlobalFlags, sampleRate);
   lame_set_disable_reservoir(mGlobalFlags, false);
   // Add the VbrTag for all types.  For ABR/VBR, a Xing tag will be created.
   // For CBR, it will be a Lame Info tag.
   lame_set_bWriteVbrTag(mGlobalFlags, true);

   // Set the VBR quality or ABR/CBR bitrate
   switch (mMode) {
      case MODE_SET:
      {
         int preset;

         switch (mQuality)
         {
            case PRESET_INSANE:
               preset = INSANE;
               break;
            case PRESET_EXTREME:
               preset = EXTREME_FAST;
               break;
            case PRESET_STANDARD:
               preset = STANDARD_FAST;
               break;
            default:
               preset = MEDIUM_FAST;
               break;
         }

         lame_set_preset(mGlobalFlags, preset);
      }
      break;

      case MODE_VBR:
         lame_set_VBR(mGlobalFlags, vbr_mtrh );
         lame_set_VBR_q(mGlobalFlags, mQuality);
      break;

      case MODE_ABR:
         lame_set_preset(mGlobalFlags, mBitrate );
      break;

      default:
         lame_set_VBR(mGlobalFlags, vbr_off);
         lame_set_brate(mGlobalFlags, mBitrate);
      break;
   }

   // Set the channel mode
   MPEG_mode mode;

   if (channels == 1 || mChannel == CHANNEL_MONO) {
      mode = MONO;
   }
   else if (mChannel == CHANNEL_JOINT) {
      mode = JOINT_STEREO;
   }
   else {
      mode = STEREO;
   }
   lame_set_mode(mGlobalFlags, mode);

   int rc = lame_init_params(mGlobalFlags);
   if (rc < 0) {
      return rc;
   }

   mInfoTagLen = 0;
   mEncoding = true;

   return mSamplesPerChunk;
}

int MP3Exporter::GetOutBufferSize()
{
   if (!mEncoding)
      return -1;

   return mOutBufferSize;
}

int MP3Exporter::EncodeBuffer(float inbuffer[], unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_interleaved_ieee_float(mGlobalFlags, inbuffer, mSamplesPerChunk,
      outbuffer, mOutBufferSize);
}

int MP3Exporter::EncodeRemainder(float inbuffer[], int nSamples,
                  unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_interleaved_ieee_float(mGlobalFlags, inbuffer, nSamples, outbuffer,
      mOutBufferSize);
}

int MP3Exporter::EncodeBufferMono(float inbuffer[], unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_ieee_float(mGlobalFlags, inbuffer,inbuffer, mSamplesPerChunk,
      outbuffer, mOutBufferSize);
}

int MP3Exporter::EncodeRemainderMono(float inbuffer[], int nSamples,
                  unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_ieee_float(mGlobalFlags, inbuffer, inbuffer, nSamples, outbuffer,
      mOutBufferSize);
}

int MP3Exporter::FinishStream(unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   mEncoding = false;

   int result = lame_encode_flush(mGlobalFlags, outbuffer, mOutBufferSize);

   mInfoTagLen = lame_get_lametag_frame(mGlobalFlags, mInfoTagBuf, sizeof(mInfoTagBuf));

   return result;
}

void MP3Exporter::CancelEncoding()
{
   mEncoding = false;
}

bool MP3Exporter::PutInfoTag(wxFFile & f, wxFileOffset off)
{
   if (mGlobalFlags) {
      if (mInfoTagLen > 0) {
         // FIXME: TRAP_ERR Seek and writ ein MP3 exporter could fail.
         if ( !f.Seek(off, wxFromStart))
            return false;
         if (mInfoTagLen > f.Write(mInfoTagBuf, mInfoTagLen))
            return false;
      }
      else {
         lame_mp3_tags_fid(mGlobalFlags, f.fp());
      }
   }

   if ( !f.SeekEnd() )
      return false;

   return true;
}

//----------------------------------------------------------------------------
// ExportMP3
//----------------------------------------------------------------------------

class ExportMP3 final : public ExportPlugin
{
public:

   ExportMP3();
   bool CheckFileName(wxFileName & filename, int format) override;

   // Required

   void OptionsCreate(ShuttleGui &S, int format) override;
   ProgressResult Export(TenacityProject *project,
               std::unique_ptr<ProgressDialog> &pDialog,
               unsigned channels,
               const wxFileNameWrapper &fName,
               bool selectedOnly,
               double t0,
               double t1,
               MixerSpec *mixerSpec = NULL,
               const Tags *metadata = NULL,
               int subformat = 0) override;

private:

   int AskResample(int bitrate, int rate, int lowrate, int highrate);
   unsigned long AddTags(TenacityProject *project, ArrayOf<char> &buffer, bool *endOfFile, const Tags *tags);
#ifdef USE_LIBID3TAG
   void AddFrame(struct id3_tag *tp, const wxString & n, const wxString & v, const char *name);
#endif
   int SetNumExportChannels() override;
};

ExportMP3::ExportMP3()
:  ExportPlugin()
{
   AddFormat();
   SetFormat(wxT("MP3"),0);
   AddExtension(wxT("mp3"),0);
   SetMaxChannels(2,0);
   SetCanMetaData(true,0);
   SetDescription(XO("MP3 Files"),0);
}

bool ExportMP3::CheckFileName(wxFileName & WXUNUSED(filename), int WXUNUSED(format))
{
   return true;
}

int ExportMP3::SetNumExportChannels()
{
   bool mono;
   gPrefs->Read(wxT("/FileFormats/MP3ForceMono"), &mono, 0);

   return (mono)? 1 : -1;
}


ProgressResult ExportMP3::Export(TenacityProject *project,
                       std::unique_ptr<ProgressDialog> &pDialog,
                       unsigned channels,
                       const wxFileNameWrapper &fName,
                       bool selectionOnly,
                       double t0,
                       double t1,
                       MixerSpec *mixerSpec,
                       const Tags *metadata,
                       int WXUNUSED(subformat))
{
   int rate = lrint( ProjectRate::Get( *project ).GetRate());
   const auto &tracks = TrackList::Get( *project );
   MP3Exporter exporter;

   // Retrieve preferences
   int highrate = 48000;
   int lowrate = 8000;
   int bitrate = 0;
   int brate;
   //int vmode;
   bool forceMono;

   gPrefs->Read(wxT("/FileFormats/MP3Bitrate"), &brate, 128);
   auto rmode = MP3RateModeSetting.ReadEnumWithDefault( MODE_CBR );
   //gPrefs->Read(wxT("/FileFormats/MP3VarMode"), &vmode, ROUTINE_FAST);
   auto cmode = MP3ChannelModeSetting.ReadEnumWithDefault( CHANNEL_STEREO );
   gPrefs->Read(wxT("/FileFormats/MP3ForceMono"), &forceMono, 0);

   // Set the bitrate/quality and mode
   if (rmode == MODE_SET) {
      brate = ValidateValue(setRateNames.size(), brate, PRESET_STANDARD);
      //int r = ValidateValue( varModeNames.size(), vmode, ROUTINE_FAST );
      exporter.SetMode(MODE_SET);
      exporter.SetQuality(brate/*, r*/);
   }
   else if (rmode == MODE_VBR) {
      brate = ValidateValue( varRateNames.size(), brate, QUALITY_2 );
      //int r = ValidateValue( varModeNames.size(), vmode, ROUTINE_FAST );
      exporter.SetMode(MODE_VBR);
      exporter.SetQuality(brate/*, r*/);
   }
   else if (rmode == MODE_ABR) {
      brate = ValidateIndex( fixRateValues, brate, 6 /* 128 kbps */ );
      bitrate = fixRateValues[ brate ];
      exporter.SetMode(MODE_ABR);
      exporter.SetBitrate(bitrate);

      if (bitrate > 160) {
         lowrate = 32000;
      }
      else if (bitrate < 32 || bitrate == 144) {
         highrate = 24000;
      }
   }
   else {
      brate = ValidateIndex( fixRateValues, brate, 6 /* 128 kbps */ );
      bitrate = fixRateValues[ brate ];
      exporter.SetMode(MODE_CBR);
      exporter.SetBitrate(bitrate);

      if (bitrate > 160) {
         lowrate = 32000;
      }
      else if (bitrate < 32 || bitrate == 144) {
         highrate = 24000;
      }
   }

   // Verify sample rate
   if (!make_iterator_range( sampRates ).contains( rate ) ||
      (rate < lowrate) || (rate > highrate)) {
      rate = AskResample(bitrate, rate, lowrate, highrate);
      if (rate == 0) {
         return ProgressResult::Cancelled;
      }
   }

   // Set the channel mode
   if (forceMono) {
      exporter.SetChannel(CHANNEL_MONO);
   }
   else if (cmode == CHANNEL_JOINT) {
      exporter.SetChannel(CHANNEL_JOINT);
   }
   else {
      exporter.SetChannel(CHANNEL_STEREO);
   }

   auto inSamples = exporter.InitializeStream(channels, rate);
   if (((int)inSamples) < 0) {
      AudacityMessageBox( XO("Unable to initialize MP3 stream") );
      return ProgressResult::Cancelled;
   }

   // Put ID3 tags at beginning of file
   if (metadata == NULL)
      metadata = &Tags::Get( *project );

   // Open file for writing
   wxFFile outFile(fName.GetFullPath(), wxT("w+b"));
   if (!outFile.IsOpened()) {
      AudacityMessageBox( XO("Unable to open target file for writing") );
      return ProgressResult::Cancelled;
   }

   ArrayOf<char> id3buffer;
   bool endOfFile;
   unsigned long id3len = AddTags(project, id3buffer, &endOfFile, metadata);
   if (id3len && !endOfFile) {
      if (id3len > outFile.Write(id3buffer.get(), id3len)) {
         // TODO: more precise message
         ShowExportErrorDialog("MP3:1882");
         return ProgressResult::Cancelled;
      }
   }

   wxFileOffset pos = outFile.Tell();
   auto updateResult = ProgressResult::Success;
   int bytes = 0;

   size_t bufferSize = std::max(0, exporter.GetOutBufferSize());
   if (bufferSize <= 0) {
      // TODO: more precise message
      ShowExportErrorDialog("MP3:1849");
      return ProgressResult::Cancelled;
   }

   ArrayOf<unsigned char> buffer{ bufferSize };
   wxASSERT(buffer);

   {
      auto mixer = CreateMixer(tracks, selectionOnly,
         t0, t1,
         channels, inSamples, true,
         rate, floatSample, mixerSpec);

      TranslatableString title;
      if (rmode == MODE_SET) {
         title = (selectionOnly ?
            XO("Exporting selected audio with %s preset") :
            XO("Exporting the audio with %s preset"))
               .Format( setRateNamesShort[brate] );
      }
      else if (rmode == MODE_VBR) {
         title = (selectionOnly ?
            XO("Exporting selected audio with VBR quality %s") :
            XO("Exporting the audio with VBR quality %s"))
               .Format( varRateNames[brate] );
      }
      else {
         title = (selectionOnly ?
            XO("Exporting selected audio at %d Kbps") :
            XO("Exporting the audio at %d Kbps"))
               .Format( bitrate );
      }

      InitProgress( pDialog, fName, title );
      auto &progress = *pDialog;

      while (updateResult == ProgressResult::Success) {
         auto blockLen = mixer->Process(inSamples);

         if (blockLen == 0) {
            break;
         }

         float *mixed = (float *)mixer->GetBuffer();

         if ((int)blockLen < inSamples) {
            if (channels > 1) {
               bytes = exporter.EncodeRemainder(mixed, blockLen, buffer.get());
            }
            else {
               bytes = exporter.EncodeRemainderMono(mixed, blockLen, buffer.get());
            }
         }
         else {
            if (channels > 1) {
               bytes = exporter.EncodeBuffer(mixed, buffer.get());
            }
            else {
               bytes = exporter.EncodeBufferMono(mixed, buffer.get());
            }
         }

         if (bytes < 0) {
            auto msg = XO("Error %ld returned from MP3 encoder")
               .Format( bytes );
            AudacityMessageBox( msg );
            updateResult = ProgressResult::Cancelled;
            break;
         }

         if (bytes > (int)outFile.Write(buffer.get(), bytes)) {
            // TODO: more precise message
            ShowDiskFullExportErrorDialog(fName);
            updateResult = ProgressResult::Cancelled;
            break;
         }

         updateResult = progress.Update(mixer->MixGetCurrentTime() - t0, t1 - t0);
      }
   }

   if ( updateResult == ProgressResult::Success ||
        updateResult == ProgressResult::Stopped ) {
      bytes = exporter.FinishStream(buffer.get());

      if (bytes < 0) {
         // TODO: more precise message
         ShowExportErrorDialog("MP3:1981");
         return ProgressResult::Cancelled;
      }

      if (bytes > 0) {
         if (bytes > (int)outFile.Write(buffer.get(), bytes)) {
            // TODO: more precise message
            ShowExportErrorDialog("MP3:1988");
            return ProgressResult::Cancelled;
         }
      }

      // Write ID3 tag if it was supposed to be at the end of the file
      if (id3len > 0 && endOfFile) {
         if (bytes > (int)outFile.Write(id3buffer.get(), id3len)) {
            // TODO: more precise message
            ShowExportErrorDialog("MP3:1997");
            return ProgressResult::Cancelled;
         }
      }

      // Always write the info (Xing/Lame) tag.  Until we stop supporting Lame
      // versions before 3.98, we must do this after the MP3 file has been
      // closed.
      //
      // Also, if beWriteInfoTag() is used, mGlobalFlags will no longer be valid after
      // this call, so do not use it.
      if (!exporter.PutInfoTag(outFile, pos) ||
          !outFile.Flush() ||
          !outFile.Close()) {
         // TODO: more precise message
         ShowExportErrorDialog("MP3:2012");
         return ProgressResult::Cancelled;
      }
   }

   return updateResult;
}

void ExportMP3::OptionsCreate(ShuttleGui &S, int format)
{
   S.AddWindow( safenew ExportMP3Options{ S.GetParent(), format } );
}

int ExportMP3::AskResample(int bitrate, int rate, int lowrate, int highrate)
{
   wxDialogWrapper d(nullptr, wxID_ANY, XO("Invalid sample rate"));
   d.SetName();
   wxChoice *choice;
   ShuttleGui S(&d, eIsCreating);

   int selected = -1;

   S.StartVerticalLay();
   {
      S.SetBorder(10);
      S.StartStatic(XO("Resample"));
      {
         S.StartHorizontalLay(wxALIGN_CENTER, false);
         {
            S.AddTitle(
               ((bitrate == 0)
                  ? XO(
"The project sample rate (%d) is not supported by the MP3\nfile format. ")
                       .Format( rate )
                  : XO(
"The project sample rate (%d) and bit rate (%d kbps) combination is not\nsupported by the MP3 file format. ")
                       .Format( rate, bitrate ))
               + XO("You may resample to one of the rates below.")
            );
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay(wxALIGN_CENTER, false);
         {
            choice = S.AddChoice(XXO("Sample Rates"),
               [&]{
                  TranslatableStrings choices;
                  for (size_t ii = 0, nn = sampRates.size(); ii < nn; ++ii) {
                     int label = sampRates[ii];
                     if (label >= lowrate && label <= highrate) {
                        choices.push_back( Verbatim( "%d" ).Format( label ) );
                        if (label <= rate)
                           selected = ii;
                     }
                  }
                  return choices;
               }(),
               std::max( 0, selected )
            );
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.AddStandardButtons();
   }
   S.EndVerticalLay();

   d.Layout();
   d.Fit();
   d.SetMinSize(d.GetSize());
   d.Center();

   if (d.ShowModal() == wxID_CANCEL) {
      return 0;
   }

   return wxAtoi(choice->GetStringSelection());
}

#ifdef USE_LIBID3TAG
struct id3_tag_deleter {
   void operator () (id3_tag *p) const { if (p) id3_tag_delete(p); }
};
using id3_tag_holder = std::unique_ptr<id3_tag, id3_tag_deleter>;
#endif

// returns buffer len; caller frees
unsigned long ExportMP3::AddTags(TenacityProject *WXUNUSED(project), ArrayOf<char> &buffer, bool *endOfFile, const Tags *tags)
{
#ifdef USE_LIBID3TAG
   id3_tag_holder tp { id3_tag_new() };

   for (const auto &pair : tags->GetRange()) {
      const auto &n = pair.first;
      const auto &v = pair.second;
      const char *name = "TXXX";

      if (n.CmpNoCase(TAG_TITLE) == 0) {
         name = ID3_FRAME_TITLE;
      }
      else if (n.CmpNoCase(TAG_ARTIST) == 0) {
         name = ID3_FRAME_ARTIST;
      }
      else if (n.CmpNoCase(TAG_ALBUM) == 0) {
         name = ID3_FRAME_ALBUM;
      }
      else if (n.CmpNoCase(TAG_YEAR) == 0) {
         // LLL:  Some apps do not like the newer frame ID (ID3_FRAME_YEAR),
         //       so we add old one as well.
         AddFrame(tp.get(), n, v, "TYER");
         name = ID3_FRAME_YEAR;
      }
      else if (n.CmpNoCase(TAG_GENRE) == 0) {
         name = ID3_FRAME_GENRE;
      }
      else if (n.CmpNoCase(TAG_COMMENTS) == 0) {
         name = ID3_FRAME_COMMENT;
      }
      else if (n.CmpNoCase(TAG_TRACK) == 0) {
         name = ID3_FRAME_TRACK;
      }

      AddFrame(tp.get(), n, v, name);
   }

   tp->options &= (~ID3_TAG_OPTION_COMPRESSION); // No compression

   // If this version of libid3tag supports it, use v2.3 ID3
   // tags instead of the newer, but less well supported, v2.4
   // that libid3tag uses by default.
   #ifdef ID3_TAG_HAS_TAG_OPTION_ID3V2_3
   tp->options |= ID3_TAG_OPTION_ID3V2_3;
   #endif

   *endOfFile = false;

   unsigned long len;

   len = id3_tag_render(tp.get(), 0);
   buffer.reinit(len);
   len = id3_tag_render(tp.get(), (id3_byte_t *)buffer.get());

   return len;
#else //ifdef USE_LIBID3TAG
   return 0;
#endif
}

#ifdef USE_LIBID3TAG
void ExportMP3::AddFrame(struct id3_tag *tp, const wxString & n, const wxString & v, const char *name)
{
   struct id3_frame *frame = id3_frame_new(name);

   if (!n.IsAscii() || !v.IsAscii()) {
      id3_field_settextencoding(id3_frame_field(frame, 0), ID3_FIELD_TEXTENCODING_UTF_16);
   }
   else {
      id3_field_settextencoding(id3_frame_field(frame, 0), ID3_FIELD_TEXTENCODING_ISO_8859_1);
   }

   MallocString<id3_ucs4_t> ucs4{
      id3_utf8_ucs4duplicate((id3_utf8_t *) (const char *) v.mb_str(wxConvUTF8)) };

   if (strcmp(name, ID3_FRAME_COMMENT) == 0) {
      // A hack to get around iTunes not recognizing the comment.  The
      // language defaults to XXX and, since it's not a valid language,
      // iTunes just ignores the tag.  So, either set it to a valid language
      // (which one???) or just clear it.  Unfortunately, there's no supported
      // way of clearing the field, so do it directly.
      struct id3_frame *frame2 = id3_frame_new(name);
      id3_field_setfullstring(id3_frame_field(frame2, 3), ucs4.get());
      id3_field *f2 = id3_frame_field(frame2, 1);
      memset(f2->immediate.value, 0, sizeof(f2->immediate.value));
      id3_tag_attachframe(tp, frame2);
      // Now install a second frame with the standard default language = "XXX"
      id3_field_setfullstring(id3_frame_field(frame, 3), ucs4.get());
   }
   else if (strcmp(name, "TXXX") == 0) {
      id3_field_setstring(id3_frame_field(frame, 2), ucs4.get());

      ucs4.reset(id3_utf8_ucs4duplicate((id3_utf8_t *) (const char *) n.mb_str(wxConvUTF8)));

      id3_field_setstring(id3_frame_field(frame, 1), ucs4.get());
   }
   else {
      auto addr = ucs4.get();
      id3_field_setstrings(id3_frame_field(frame, 1), 1, &addr);
   }

   id3_tag_attachframe(tp, frame);
}
#endif

static Exporter::RegisteredExportPlugin sRegisteredPlugin{ "MP3",
   []{ return std::make_unique< ExportMP3 >(); }
};
