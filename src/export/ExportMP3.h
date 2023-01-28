/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMP3.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EXPORTMP3__
#define __AUDACITY_EXPORTMP3__

/* --------------------------------------------------------------------------*/

#include <memory>

enum MP3RateMode : unsigned {
   MODE_SET = 0,
   MODE_VBR,
   MODE_ABR,
   MODE_CBR,
};

template< typename Enum > class EnumSetting;
extern EnumSetting< MP3RateMode > MP3RateModeSetting;

class TranslatableString;
class wxWindow;

#endif

