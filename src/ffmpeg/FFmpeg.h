/**********************************************************************

Audacity: A Digital Audio Editor

FFmpeg.h

Audacity(R) is copyright (c) 1999-2009 Audacity Team.
License: GPL v2.  See License.txt.

******************************************************************//**

Describes shared object that is used to access FFmpeg libraries.

*//*******************************************************************/

#ifndef __AUDACITY_FFMPEG__
#define __AUDACITY_FFMPEG__


#include "audacity/Types.h"
#include "widgets/wxPanelWrapper.h" // to inherit

class wxCheckBox;

/* rather earlier than normal, but pulls in config*.h and other program stuff
 * we need for the next bit */
#include "shuttle/ShuttleGui.h"
#include "Prefs.h"

#include "Identifier.h"
#include "SampleFormat.h"

#ifdef USE_FFMPEG
#   include "FFmpegFunctions.h"
#endif

TranslatableString GetFFmpegVersion();

/* from here on in, this stuff only applies when ffmpeg is available */
#ifdef USE_FFMPEG

//----------------------------------------------------------------------------
// Attempt to load and enable/disable FFmpeg at startup
//----------------------------------------------------------------------------
void FFmpegStartup();

bool LoadFFmpeg(bool showerror);

bool FindFFmpegLibs(wxWindow* parent = nullptr);

extern BoolSetting FFmpegNotFoundDontShow;

#endif // USE_FFMPEG
#endif // __AUDACITY_FFMPEG__

