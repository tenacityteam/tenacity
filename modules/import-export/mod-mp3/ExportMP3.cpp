/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMP3.cpp

  Joshua Haberman

  This just acts as an interface to LAME. A Lame dynamic library must
  be present

  The difficulty in our approach is that we are attempting to use LAME
  in a way it was not designed to be used. LAME's API is reasonably
  consistent, so if we were linking directly against it we could expect
  this code to work with a variety of different LAME versions. However,
  the data structures change from version to version, and so linking
  with one version of the header and dynamically linking against a
  different version of the dynamic library will not work correctly.

  The solution is to find the lowest common denominator between versions.
  The bare minimum of functionality we must use is this:
      1. Initialize the library.
      2. Set, at minimum, the following global options:
          i.  input sample rate
          ii. input channels
      3. Encode the stream
      4. Call the finishing routine

  Just so that it's clear that we're NOT free to use whatever features
  of LAME we like, I'm not including lame.h, but instead enumerating
  here the extent of functions and structures that we can rely on being
  able to import and use from a dynamic library.

  For the record, we aim to support LAME 3.70 on. Since LAME 3.70 was
  released in April of 2000, that should be plenty.


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

#include <wx/app.h>
#include <wx/defs.h>

#include <wx/dynlib.h>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/mimetype.h>

#include <wx/textctrl.h>
#include <wx/choice.h>

#include <rapidjson/document.h>

#include "FileNames.h"
#include "float_cast.h"
#include "HelpSystem.h"
#include "Mix.h"
#include "Prefs.h"
#include "Tags.h"
#include "Track.h"
#include "wxFileNameWrapper.h"
#include "wxPanelWrapper.h"
#include "Project.h"

#include "Export.h"
#include "BasicUI.h"

#include <lame/lame.h>

#ifdef USE_LIBID3TAG
#include <id3tag.h>
#endif

#include "ExportOptionsEditor.h"
#include "ExportPluginHelpers.h"
#include "ExportPluginRegistry.h"
#include "SelectFile.h"
#include "ShuttleGui.h"

//----------------------------------------------------------------------------
// ExportMP3Options
//----------------------------------------------------------------------------

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

static const std::vector<ExportValue> fixRateValues {
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
   XO("Excessive, 320 kbps"),
   XO("Extreme, 220-260 kbps"),
   XO("Standard, 170-210 kbps"),
   XO("Medium, 145-185 kbps"),
};

static const TranslatableStrings setRateNamesShort {
   XO("Excessive"),
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

enum MP3OptionID : int {
   MP3OptionIDMode = 0,
   MP3OptionIDQualitySET,
   MP3OptionIDQualityVBR,
   MP3OptionIDQualityABR,
   MP3OptionIDQualityCBR
};

//Option order should exactly match to the id values
const std::initializer_list<ExportOption> MP3Options {
   {
      MP3OptionIDMode, XO("Bit Rate Mode"),
      std::string("SET"),
      ExportOption::TypeEnum,
      {
         // for migrating old preferences the
         // order should be preserved
         std::string("SET"),
         std::string("VBR"),
         std::string("ABR"),
         std::string("CBR")
      },
      {
         XO("Preset"),
         XO("Variable"),
         XO("Average"),
         XO("Constant")
      }
   },
   {
      MP3OptionIDQualitySET, XO("Quality"),
      PRESET_STANDARD,
      ExportOption::TypeEnum,
      { 0, 1, 2, 3 },
      setRateNames
   },
   {
      MP3OptionIDQualityVBR, XO("Quality"),
      QUALITY_2,
      ExportOption::TypeEnum | ExportOption::Hidden,
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
      varRateNames
   },
   {
      MP3OptionIDQualityABR, XO("Quality"),
      192,
      ExportOption::TypeEnum | ExportOption::Hidden,
      fixRateValues,
      fixRateNames
   },
   {
      MP3OptionIDQualityCBR, XO("Quality"),
      192,
      ExportOption::TypeEnum | ExportOption::Hidden,
      fixRateValues,
      fixRateNames
   }
};

class MP3ExportOptionsEditor final : public ExportOptionsEditor
{
   std::vector<ExportOption> mOptions;
   std::unordered_map<int, ExportValue> mValues;
   Listener* mListener{nullptr};
public:

   explicit MP3ExportOptionsEditor(Listener* listener)
      : mOptions(MP3Options)
      , mListener(listener)
   {
      mValues.reserve(mOptions.size());
      for(auto& option : mOptions)
         mValues[option.id] = option.defaultValue;
   }

   int GetOptionsCount() const override
   {
      return static_cast<int>(mOptions.size());
   }

   bool GetOption(int index, ExportOption& option) const override
   {
      if(index >= 0 && index < static_cast<int>(mOptions.size()))
      {
         option = mOptions[index];
         return true;
      }
      return false;
   }

   bool SetValue(int id, const ExportValue& value) override
   {
      const auto it = mValues.find(id);
      if(it == mValues.end())
         return false;
      if(value.index() != it->second.index())
         return false;

      it->second = value;

      switch(id)
      {
      case MP3OptionIDMode:
         {
            const auto mode = *std::get_if<std::string>(&value);
            OnModeChange(mode);
            if(mListener)
            {
               mListener->OnExportOptionChangeBegin();
               mListener->OnExportOptionChange(mOptions[MP3OptionIDQualitySET]);
               mListener->OnExportOptionChange(mOptions[MP3OptionIDQualityABR]);
               mListener->OnExportOptionChange(mOptions[MP3OptionIDQualityCBR]);
               mListener->OnExportOptionChange(mOptions[MP3OptionIDQualityVBR]);
               mListener->OnExportOptionChangeEnd();

               mListener->OnSampleRateListChange();
            }
         } break;
      case MP3OptionIDQualityABR:
      case MP3OptionIDQualityCBR:
      case MP3OptionIDQualitySET:
      case MP3OptionIDQualityVBR:
         {
            if(mListener)
               mListener->OnSampleRateListChange();
         } break;
      default: break;
      }
      return true;
   }

   bool GetValue(int id, ExportValue& value) const override
   {
      const auto it = mValues.find(id);
      if(it != mValues.end())
      {
         value = it->second;
         return true;
      }
      return false;
   }

   SampleRateList GetSampleRateList() const override
   {
      // Retrieve preferences
      int highrate = 48000;
      int lowrate = 8000;

      const auto rmode = *std::get_if<std::string>(&mValues.find(MP3OptionIDMode)->second);

      if (rmode == "ABR") {
         auto bitrate = *std::get_if<int>(&mValues.find(MP3OptionIDQualityABR)->second);
         if (bitrate > 160) {
            lowrate = 32000;
         }
         else if (bitrate < 32 || bitrate == 144) {
            highrate = 24000;
         }
      }
      else if (rmode == "CBR") {
         auto bitrate = *std::get_if<int>(&mValues.find(MP3OptionIDQualityCBR)->second);

         if (bitrate > 160) {
            lowrate = 32000;
         }
         else if (bitrate < 32 || bitrate == 144) {
            highrate = 24000;
         }
      }

      SampleRateList result;
      result.reserve(sampRates.size());
      for(auto rate : sampRates)
         if(rate >= lowrate && rate <= highrate)
            result.push_back(rate);

      return result;
   }

   void Load(const audacity::BasicSettings& config) override
   {
      wxString mode;
      if(config.Read(wxT("/FileFormats/MP3RateModeChoice"), &mode))
         mValues[MP3OptionIDMode] = mode.ToStdString();
      else
      {
         //attempt to recover from old-style preference
         int index;
         if(config.Read(wxT("/FileFormats/MP3RateMode"), &index))
            mValues[MP3OptionIDMode] = mOptions[MP3OptionIDMode].values[index];
      }

      config.Read(wxT("/FileFormats/MP3SetRate"), std::get_if<int>(&mValues[MP3OptionIDQualitySET]));
      config.Read(wxT("/FileFormats/MP3AbrRate"), std::get_if<int>(&mValues[MP3OptionIDQualityABR]));
      config.Read(wxT("/FileFormats/MP3CbrRate"), std::get_if<int>(&mValues[MP3OptionIDQualityCBR]));
      config.Read(wxT("/FileFormats/MP3VbrRate"), std::get_if<int>(&mValues[MP3OptionIDQualityVBR]));

      OnModeChange(*std::get_if<std::string>(&mValues[MP3OptionIDMode]));
   }

   void Store(audacity::BasicSettings& config) const override
   {
      auto it = mValues.find(MP3OptionIDMode);
      config.Write(wxT("/FileFormats/MP3RateModeChoice"), wxString(*std::get_if<std::string>(&it->second)));

      it = mValues.find(MP3OptionIDQualitySET);
      config.Write(wxT("/FileFormats/MP3SetRate"), *std::get_if<int>(&it->second));
      it = mValues.find(MP3OptionIDQualityABR);
      config.Write(wxT("/FileFormats/MP3AbrRate"), *std::get_if<int>(&it->second));
      it = mValues.find(MP3OptionIDQualityCBR);
      config.Write(wxT("/FileFormats/MP3CbrRate"), *std::get_if<int>(&it->second));
      it = mValues.find(MP3OptionIDQualityVBR);
      config.Write(wxT("/FileFormats/MP3VbrRate"), *std::get_if<int>(&it->second));
   }

private:

   void OnModeChange(const std::string& mode)
   {
      mOptions[MP3OptionIDQualitySET].flags |= ExportOption::Hidden;
      mOptions[MP3OptionIDQualityABR].flags |= ExportOption::Hidden;
      mOptions[MP3OptionIDQualityCBR].flags |= ExportOption::Hidden;
      mOptions[MP3OptionIDQualityVBR].flags |= ExportOption::Hidden;

      if(mode == "SET")
         mOptions[MP3OptionIDQualitySET].flags &= ~ExportOption::Hidden;
      else if(mode == "ABR")
         mOptions[MP3OptionIDQualityABR].flags &= ~ExportOption::Hidden;
      else if(mode == "CBR")
         mOptions[MP3OptionIDQualityCBR].flags &= ~ExportOption::Hidden;
      else if(mode == "VBR")
         mOptions[MP3OptionIDQualityVBR].flags &= ~ExportOption::Hidden;
   }
};

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

//----------------------------------------------------------------------------
// FindDialog
//----------------------------------------------------------------------------

#define ID_BROWSE 5000
#define ID_DLOAD  5001

class FindDialog final : public wxDialogWrapper
{
public:

private:

   wxTextCtrl *mPathText;

   DECLARE_EVENT_TABLE()
};

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

   /* initialize the library interface */
   bool Init();

   /* get library info */
   wxString GetLibraryVersion();

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
   bool mEncoding;
   int mMode;
   int mBitrate;
   int mQuality;
   //int mRoutine;

   lame_global_flags *mGF;

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
   mGF = NULL;

   mBitrate = 128;
   mQuality = QUALITY_2;
   mMode = MODE_CBR;
   //mRoutine = ROUTINE_FAST;
}

MP3Exporter::~MP3Exporter()
{
   if (mGF)
   {
      lame_close(mGF);
      mGF = NULL;
   }
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
}

bool MP3Exporter::Init()
{
   mGF = lame_init();
   if (mGF == NULL) {
      return false;
   }

   return true;
}

wxString MP3Exporter::GetLibraryVersion()
{
   return wxString::Format(wxT("LAME %hs"), get_lame_version());
}

int MP3Exporter::InitializeStream(unsigned channels, int sampleRate)
{
   if (channels > 2) {
      return -1;
   }

   lame_set_error_protection(mGF, false);
   lame_set_num_channels(mGF, channels);
   lame_set_in_samplerate(mGF, sampleRate);
   lame_set_out_samplerate(mGF, sampleRate);
   lame_set_disable_reservoir(mGF, false);
   // Add the VbrTag for all types.  For ABR/VBR, a Xing tag will be created.
   // For CBR, it will be a Lame Info tag.
   lame_set_bWriteVbrTag(mGF, true);

   // Set the VBR quality or ABR/CBR bitrate
   switch (mMode) {
      case MODE_SET:
      {
         int preset;

         if (mQuality == PRESET_INSANE) {
            preset = INSANE;
         }
         //else if (mRoutine == ROUTINE_FAST) {
            else if (mQuality == PRESET_EXTREME) {
               preset = EXTREME_FAST;
            }
            else if (mQuality == PRESET_STANDARD) {
               preset = STANDARD_FAST;
            }
            else {
               preset = 1007;    // Not defined until 3.96
            }
         //}
         /*
         else {
            if (mQuality == PRESET_EXTREME) {
               preset = EXTREME;
            }
            else if (mQuality == PRESET_STANDARD) {
               preset = STANDARD;
            }
            else {
               preset = 1006;    // Not defined until 3.96
            }
         }
         */
         lame_set_preset(mGF, preset);
      }
      break;

      case MODE_VBR:
         lame_set_VBR(mGF, vbr_mtrh );
         lame_set_VBR_q(mGF, mQuality);
      break;

      case MODE_ABR:
         lame_set_preset(mGF, mBitrate );
      break;

      default:
         lame_set_VBR(mGF, vbr_off);
         lame_set_brate(mGF, mBitrate);
      break;
   }

   // Set the channel mode
   MPEG_mode mode;

   if (channels == 1)
      mode = MONO;
   else
      mode = JOINT_STEREO;

   lame_set_mode(mGF, mode);

   int rc = lame_init_params(mGF);
   if (rc < 0) {
      return rc;
   }

#if 0
   dump_config(mGF);
#endif

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

   return lame_encode_buffer_interleaved_ieee_float(mGF, inbuffer, mSamplesPerChunk,
      outbuffer, mOutBufferSize);
}

int MP3Exporter::EncodeRemainder(float inbuffer[], int nSamples,
                  unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_interleaved_ieee_float(mGF, inbuffer, nSamples, outbuffer,
      mOutBufferSize);
}

int MP3Exporter::EncodeBufferMono(float inbuffer[], unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_ieee_float(mGF, inbuffer,inbuffer, mSamplesPerChunk,
      outbuffer, mOutBufferSize);
}

int MP3Exporter::EncodeRemainderMono(float inbuffer[], int nSamples,
                  unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   return lame_encode_buffer_ieee_float(mGF, inbuffer, inbuffer, nSamples, outbuffer,
      mOutBufferSize);
}

int MP3Exporter::FinishStream(unsigned char outbuffer[])
{
   if (!mEncoding) {
      return -1;
   }

   mEncoding = false;

   int result = lame_encode_flush(mGF, outbuffer, mOutBufferSize);
   mInfoTagLen = lame_get_lametag_frame(mGF, mInfoTagBuf, sizeof(mInfoTagBuf));

   return result;
}

void MP3Exporter::CancelEncoding()
{
   mEncoding = false;
}

bool MP3Exporter::PutInfoTag(wxFFile & f, wxFileOffset off)
{
   if (mGF) {
      if (mInfoTagLen > 0) {
         // FIXME: TRAP_ERR Seek and writ ein MP3 exporter could fail.
         if ( !f.Seek(off, wxFromStart))
            return false;
         if (mInfoTagLen > f.Write(mInfoTagBuf, mInfoTagLen))
            return false;
      } else {
         lame_mp3_tags_fid(mGF, f.fp());
      }
   }

   if ( !f.SeekEnd() )
      return false;

   return true;
}

#if 0
// Debug routine from BladeMP3EncDLL.c in the libmp3lame distro
static void dump_config( 	lame_global_flags*	gfp )
{
   wxPrintf(wxT("\n\nLame_enc configuration options:\n"));
   wxPrintf(wxT("==========================================================\n"));

   wxPrintf(wxT("version                =%d\n"),lame_get_version( gfp ) );
   wxPrintf(wxT("Layer                  =3\n"));
   wxPrintf(wxT("mode                   ="));
   switch ( lame_get_mode( gfp ) )
   {
      case STEREO:       wxPrintf(wxT( "Stereo\n" )); break;
      case JOINT_STEREO: wxPrintf(wxT( "Joint-Stereo\n" )); break;
      case DUAL_CHANNEL: wxPrintf(wxT( "Forced Stereo\n" )); break;
      case MONO:         wxPrintf(wxT( "Mono\n" )); break;
      case NOT_SET:      /* FALLTHROUGH */
      default:           wxPrintf(wxT( "Error (unknown)\n" )); break;
   }

   wxPrintf(wxT("Input sample rate      =%.1f kHz\n"), lame_get_in_samplerate( gfp ) /1000.0 );
   wxPrintf(wxT("Output sample rate     =%.1f kHz\n"), lame_get_out_samplerate( gfp ) /1000.0 );

   wxPrintf(wxT("bitrate                =%d kbps\n"), lame_get_brate( gfp ) );
   wxPrintf(wxT("Quality Setting        =%d\n"), lame_get_quality( gfp ) );

   wxPrintf(wxT("Low pass frequency     =%d\n"), lame_get_lowpassfreq( gfp ) );
   wxPrintf(wxT("Low pass width         =%d\n"), lame_get_lowpasswidth( gfp ) );

   wxPrintf(wxT("High pass frequency    =%d\n"), lame_get_highpassfreq( gfp ) );
   wxPrintf(wxT("High pass width        =%d\n"), lame_get_highpasswidth( gfp ) );

   wxPrintf(wxT("No short blocks        =%d\n"), lame_get_no_short_blocks( gfp ) );
   wxPrintf(wxT("Force short blocks     =%d\n"), lame_get_force_short_blocks( gfp ) );

   wxPrintf(wxT("de-emphasis            =%d\n"), lame_get_emphasis( gfp ) );
   wxPrintf(wxT("private flag           =%d\n"), lame_get_extension( gfp ) );

   wxPrintf(wxT("copyright flag         =%d\n"), lame_get_copyright( gfp ) );
   wxPrintf(wxT("original flag          =%d\n"),	lame_get_original( gfp ) );
   wxPrintf(wxT("CRC                    =%s\n"), lame_get_error_protection( gfp ) ? wxT("on") : wxT("off") );
   wxPrintf(wxT("Fast mode              =%s\n"), ( lame_get_quality( gfp ) )? wxT("enabled") : wxT("disabled") );
   wxPrintf(wxT("Force mid/side stereo  =%s\n"), ( lame_get_force_ms( gfp ) )?wxT("enabled"):wxT("disabled") );
   wxPrintf(wxT("Padding Type           =%d\n"), (int) lame_get_padding_type( gfp ) );
   wxPrintf(wxT("Disable Reservoir      =%d\n"), lame_get_disable_reservoir( gfp ) );
   wxPrintf(wxT("Allow diff-short       =%d\n"), lame_get_allow_diff_short( gfp ) );
   wxPrintf(wxT("Interchannel masking   =%d\n"), lame_get_interChRatio( gfp ) ); // supposed to be a float, but in lib-src/lame/lame/lame.h it's int
   wxPrintf(wxT("Strict ISO Encoding    =%s\n"), ( lame_get_strict_ISO( gfp ) ) ?wxT("Yes"):wxT("No"));
   wxPrintf(wxT("Scale                  =%5.2f\n"), lame_get_scale( gfp ) );

   wxPrintf(wxT("VBR                    =%s, VBR_q =%d, VBR method ="),
            ( lame_get_VBR( gfp ) !=vbr_off ) ? wxT("enabled"): wxT("disabled"),
            lame_get_VBR_q( gfp ) );

   switch ( lame_get_VBR( gfp ) )
   {
      case vbr_off:	wxPrintf(wxT( "vbr_off\n" ));	break;
      case vbr_mt :	wxPrintf(wxT( "vbr_mt \n" ));	break;
      case vbr_rh :	wxPrintf(wxT( "vbr_rh \n" ));	break;
      case vbr_mtrh:	wxPrintf(wxT( "vbr_mtrh \n" ));	break;
      case vbr_abr:
         wxPrintf(wxT( "vbr_abr (average bitrate %d kbps)\n"), lame_get_VBR_mean_bitrate_kbps( gfp ) );
         break;
      default:
         wxPrintf(wxT("error, unknown VBR setting\n"));
         break;
   }

   wxPrintf(wxT("Vbr Min bitrate        =%d kbps\n"), lame_get_VBR_min_bitrate_kbps( gfp ) );
   wxPrintf(wxT("Vbr Max bitrate        =%d kbps\n"), lame_get_VBR_max_bitrate_kbps( gfp ) );

   wxPrintf(wxT("Write VBR Header       =%s\n"), ( lame_get_bWriteVbrTag( gfp ) ) ?wxT("Yes"):wxT("No"));
   wxPrintf(wxT("VBR Hard min           =%d\n"), lame_get_VBR_hard_min( gfp ) );

   wxPrintf(wxT("ATH Only               =%d\n"), lame_get_ATHonly( gfp ) );
   wxPrintf(wxT("ATH short              =%d\n"), lame_get_ATHshort( gfp ) );
   wxPrintf(wxT("ATH no                 =%d\n"), lame_get_noATH( gfp ) );
   wxPrintf(wxT("ATH type               =%d\n"), lame_get_ATHtype( gfp ) );
   wxPrintf(wxT("ATH lower              =%f\n"), lame_get_ATHlower( gfp ) );
   wxPrintf(wxT("ATH aa                 =%d\n"), lame_get_athaa_type( gfp ) );
   wxPrintf(wxT("ATH aa  loudapprox     =%d\n"), lame_get_athaa_loudapprox( gfp ) );
   wxPrintf(wxT("ATH aa  sensitivity    =%f\n"), lame_get_athaa_sensitivity( gfp ) );

   wxPrintf(wxT("Experimental nspsytune =%d\n"), lame_get_exp_nspsytune( gfp ) );
   wxPrintf(wxT("Experimental X         =%d\n"), lame_get_experimentalX( gfp ) );
   wxPrintf(wxT("Experimental Y         =%d\n"), lame_get_experimentalY( gfp ) );
   wxPrintf(wxT("Experimental Z         =%d\n"), lame_get_experimentalZ( gfp ) );
}
#endif

class MP3ExportProcessor final : public ExportProcessor
{
   struct
   {
      TranslatableString status;
      unsigned channels;
      double t0;
      double t1;
      MP3Exporter exporter;
      wxFFile outFile;
      ArrayOf<char> id3buffer;
      unsigned long id3len;
      wxFileOffset infoTagPos;
      size_t bufferSize;
      int inSamples;
      std::unique_ptr<Mixer> mixer;
   } context;

public:
   bool Initialize(AudacityProject& project,
      const Parameters& parameters,
      const wxFileNameWrapper& filename,
      double t0, double t1, bool selectedOnly,
      double sampleRate, unsigned channels,
      MixerOptions::Downmix* mixerSpec,
      const Tags* tags) override;

   ExportResult Process(ExportProcessorDelegate& delegate) override;

private:

   static int AskResample(int bitrate, int rate, int lowrate, int highrate);
   static unsigned long AddTags(ArrayOf<char> &buffer, bool *endOfFile, const Tags *tags);
#ifdef USE_LIBID3TAG
   static void AddFrame(struct id3_tag *tp, const wxString & n, const wxString & v, const char *name);
#endif
};

//----------------------------------------------------------------------------
// ExportMP3
//----------------------------------------------------------------------------

class ExportMP3 final : public ExportPlugin
{
public:

   ExportMP3();
   bool CheckFileName(wxFileName & filename, int format) const override;

   int GetFormatCount() const override;
   FormatInfo GetFormatInfo(int) const override;

   std::unique_ptr<ExportOptionsEditor>
   CreateOptionsEditor(int, ExportOptionsEditor::Listener* listener) const override;

   std::unique_ptr<ExportProcessor> CreateProcessor(int format) const override;

   std::vector<std::string> GetMimeTypes(int) const override;

   bool ParseConfig(
      int formatIndex, const rapidjson::Value& document,
      ExportProcessor::Parameters& parameters) const override;
};

ExportMP3::ExportMP3() = default;

int ExportMP3::GetFormatCount() const
{
   return 1;
}

FormatInfo ExportMP3::GetFormatInfo(int) const
{
   return {
      wxT("MP3"), XO("MP3 Files"), { wxT("mp3") }, 2u, true
   };
}

std::unique_ptr<ExportOptionsEditor>
ExportMP3::CreateOptionsEditor(int, ExportOptionsEditor::Listener* listener) const
{
   return std::make_unique<MP3ExportOptionsEditor>(listener);
}

std::unique_ptr<ExportProcessor> ExportMP3::CreateProcessor(int format) const
{
   return std::make_unique<MP3ExportProcessor>();
}

std::vector<std::string> ExportMP3::GetMimeTypes(int) const
{
   return { "audio/mpeg" };
}

bool ExportMP3::ParseConfig(
   int formatIndex, const rapidjson::Value& document,
   ExportProcessor::Parameters& parameters) const
{
   if (!document.IsObject())
      return false;

   MP3OptionID qualityMode;

   if (document.HasMember("mode"))
   {
      auto& mode = document["mode"];
      if (!mode.IsString())
         return false;

      auto value = mode.GetString();

      if (value == std::string_view { "SET" })
         qualityMode = MP3OptionIDQualitySET;
      else if (value == std::string_view { "VBR" })
         qualityMode = MP3OptionIDQualityVBR;
      else if (value == std::string_view { "ABR" })
         qualityMode = MP3OptionIDQualityABR;
      else if (value == std::string_view { "CBR" })
         qualityMode = MP3OptionIDQualityCBR;
      else
         return false;

      parameters.push_back(std::make_tuple(MP3OptionIDMode, value));
   }
   else
      return false;

   if (document.HasMember("quality"))
   {
      auto& qualityMember = document["quality"];

      if (!qualityMember.IsInt())
         return false;

      const auto quality = qualityMember.GetInt();

      if (qualityMode == MP3OptionIDQualitySET && (quality < 0 || quality > 3))
         return false;
      else if (
         qualityMode == MP3OptionIDQualityVBR && (quality < 0 || quality > 9))
         return false;
      else if (
         qualityMode == MP3OptionIDQualityABR &&
         std::find(
            fixRateValues.begin(), fixRateValues.end(),
            ExportValue { quality }) ==
            fixRateValues.end())
         return false;
      else if (
         qualityMode == MP3OptionIDQualityCBR &&
         std::find(
            fixRateValues.begin(), fixRateValues.end(),
            ExportValue { quality }) ==
            fixRateValues.end())
         return false;

      parameters.push_back(std::make_tuple(qualityMode, quality));
   }
   else
      return false;

   return true;
}

bool ExportMP3::CheckFileName(wxFileName & WXUNUSED(filename), int WXUNUSED(format)) const
{
   return true;
}

bool MP3ExportProcessor::Initialize(AudacityProject& project,
   const Parameters& parameters,
   const wxFileNameWrapper& fName,
   double t0, double t1, bool selectionOnly,
   double sampleRate, unsigned channels,
   MixerOptions::Downmix* mixerSpec,
   const Tags* metadata)
{
   context.t0 = t0;
   context.t1 = t1;
   context.channels = channels;

   int rate = lrint(sampleRate);
   auto& exporter = context.exporter;

   if (!exporter.Init()) {
      throw ExportException(_("Could not initialize MP3 encoding library!"));
   }

   // Retrieve preferences
   int highrate = 48000;
   int lowrate = 8000;
   int bitrate = 0;
   int quality;

   auto rmode = ExportPluginHelpers::GetParameterValue(
      parameters,
      MP3OptionIDMode,
      std::string("CBR"));
   // Set the bitrate/quality and mode
   if (rmode == "SET") {
      quality = ExportPluginHelpers::GetParameterValue<int>(
         parameters,
         MP3OptionIDQualitySET,
         PRESET_STANDARD);
      exporter.SetMode(MODE_SET);
      exporter.SetQuality(quality);
   }
   else if (rmode == "VBR") {
      quality = ExportPluginHelpers::GetParameterValue<int>(
         parameters,
         MP3OptionIDQualityVBR,
         QUALITY_2);
      exporter.SetMode(MODE_VBR);
      exporter.SetQuality(quality);
   }
   else if (rmode == "ABR") {
      bitrate = ExportPluginHelpers::GetParameterValue(
         parameters,
         MP3OptionIDQualityABR,
         128);
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
      bitrate = ExportPluginHelpers::GetParameterValue(parameters, MP3OptionIDQualityCBR, 128);
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
        // Force valid sample rate in macros.
		if (project.mBatchMode) {
			if (!make_iterator_range( sampRates ).contains( rate )) {
				auto const bestRateIt = std::lower_bound(sampRates.begin(),
				sampRates.end(), rate);
				rate = (bestRateIt == sampRates.end()) ? highrate : *bestRateIt;
			}
			if (rate < lowrate) {
				rate = lowrate;
			}
			else if (rate > highrate) {
				rate = highrate;
			}
		}
		// else validate or prompt
		else {
			if (!make_iterator_range( sampRates ).contains( rate ) ||
				(rate < lowrate) || (rate > highrate)) {
            //This call should go away once export project rate option
            //is available as an export dialog option
			   rate = AskResample(bitrate, rate, lowrate, highrate);
			}
			if (rate == 0) {
            return false;
			}
		}
	}

   context.inSamples = exporter.InitializeStream(channels, rate);
   if (context.inSamples < 0) {
      throw ExportException(_("Unable to initialize MP3 stream"));
   }

   // Put ID3 tags at beginning of file
   if (metadata == nullptr)
      metadata = &Tags::Get( project );

   // Open file for writing
   if (!context.outFile.Open(fName.GetFullPath(), wxT("w+b"))) {
      throw ExportException(_("Unable to open target file for writing"));
   }

   bool endOfFile;
   context.id3len = AddTags(context.id3buffer, &endOfFile, metadata);
   if (context.id3len && !endOfFile) {
      if (context.id3len > context.outFile.Write(context.id3buffer.get(), context.id3len)) {
         // TODO: more precise message
         throw ExportErrorException("MP3:1882");
      }
      context.id3len = 0;
      context.id3buffer.reset();
   }

   context.infoTagPos = context.outFile.Tell();

   context.bufferSize = std::max(0, exporter.GetOutBufferSize());
   if (context.bufferSize == 0) {
      // TODO: more precise message
      throw ExportErrorException("MP3:1849");
   }

   if (rmode == "SET") {
      context.status = (selectionOnly ?
         XO("Exporting selected audio with %s preset") :
         XO("Exporting the audio with %s preset"))
            .Format( setRateNamesShort[quality] );
   }
   else if (rmode == "VBR") {
      context.status = (selectionOnly ?
         XO("Exporting selected audio with VBR quality %s") :
         XO("Exporting the audio with VBR quality %s"))
            .Format( varRateNames[quality] );
   }
   else {
      context.status = (selectionOnly ?
         XO("Exporting selected audio at %d Kbps") :
         XO("Exporting the audio at %d Kbps"))
            .Format( bitrate );
   }

   context.mixer = ExportPluginHelpers::CreateMixer(
      project, selectionOnly, t0, t1, channels, context.inSamples, true, rate,
      floatSample, mixerSpec);

   return true;
}

ExportResult MP3ExportProcessor::Process(ExportProcessorDelegate& delegate)
{
   delegate.SetStatusString(context.status);

   auto& exporter = context.exporter;
   int bytes = 0;

   ArrayOf<unsigned char> buffer{ context.bufferSize };
   wxASSERT(buffer);

   auto exportResult = ExportResult::Success;

   {
      while (exportResult == ExportResult::Success) {
         auto blockLen = context.mixer->Process();
         if (blockLen == 0)
            break;

         float *mixed = (float *)context.mixer->GetBuffer();

         if ((int)blockLen < context.inSamples) {
            if (context.channels > 1) {
               bytes = exporter.EncodeRemainder(mixed, blockLen, buffer.get());
            }
            else {
               bytes = exporter.EncodeRemainderMono(mixed, blockLen, buffer.get());
            }
         }
         else {
            if (context.channels > 1) {
               bytes = exporter.EncodeBuffer(mixed, buffer.get());
            }
            else {
               bytes = exporter.EncodeBufferMono(mixed, buffer.get());
            }
         }

         if (bytes < 0) {
            throw ExportException(XO("Error %ld returned from MP3 encoder")
               .Format( bytes )
               .Translation());
         }

         if (bytes > (int)context.outFile.Write(buffer.get(), bytes)) {
            // TODO: more precise message
            throw ExportDiskFullError(context.outFile.GetName());
         }

         if(exportResult == ExportResult::Success)
            exportResult = ExportPluginHelpers::UpdateProgress(
               delegate, *context.mixer, context.t0, context.t1);
      }
   }

   if (exportResult == ExportResult::Success) {
      bytes = exporter.FinishStream(buffer.get());

      if (bytes < 0) {
         // TODO: more precise message
         throw ExportErrorException("MP3:1981");
      }

      if (bytes > 0) {
         if (bytes > (int)context.outFile.Write(buffer.get(), bytes)) {
            // TODO: more precise message
            throw ExportErrorException("MP3:1988");
         }
      }

      // Write ID3 tag if it was supposed to be at the end of the file
      if (context.id3len > 0) {
         if (bytes > (int)context.outFile.Write(context.id3buffer.get(), context.id3len)) {
            // TODO: more precise message
            throw ExportErrorException("MP3:1997");
         }
      }

      // Always write the info (Xing/Lame) tag.  Until we stop supporting Lame
      // versions before 3.98, we must do this after the MP3 file has been
      // closed.
      //
      // Also, if beWriteInfoTag() is used, mGF will no longer be valid after
      // this call, so do not use it.
      if (!exporter.PutInfoTag(context.outFile, context.infoTagPos) ||
          !context.outFile.Flush() ||
          !context.outFile.Close()) {
         // TODO: more precise message
         throw ExportErrorException("MP3:2012");
      }
   }
   return exportResult;
}

int MP3ExportProcessor::AskResample(int bitrate, int rate, int lowrate, int highrate)
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
unsigned long MP3ExportProcessor::AddTags(ArrayOf<char> &buffer, bool *endOfFile, const Tags *tags)
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
void MP3ExportProcessor::AddFrame(struct id3_tag *tp, const wxString & n, const wxString & v, const char *name)
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

static ExportPluginRegistry::RegisteredPlugin sRegisteredPlugin{ "MP3",
   []{ return std::make_unique< ExportMP3 >(); }
};

//----------------------------------------------------------------------------
// Return library version
//----------------------------------------------------------------------------

TranslatableString GetMP3Version(wxWindow *parent, bool prompt)
{
   MP3Exporter exporter;
   auto versionString = XO("MP3 export library not found");
   versionString = Verbatim( exporter.GetLibraryVersion() );

   return versionString;
}

