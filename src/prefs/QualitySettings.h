/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 QualitySettings.h
 
 Paul Licameli split from QualityPrefs.h
 
 **********************************************************************/

#ifndef __AUDACITY_QUALITY_SETTINGS__
#define __AUDACITY_QUALITY_SETTINGS__

// Tenacity libraries
#include <lib-preferences/Prefs.h> // for EnumSetting
#include <lib-math/SampleFormat.h>

class IntSetting;

namespace QualitySettings {
extern SAUCEDACITY_DLL_API IntSetting DefaultSampleRate;
extern SAUCEDACITY_DLL_API EnumSetting< sampleFormat > SampleFormatSetting;
extern SAUCEDACITY_DLL_API sampleFormat SampleFormatChoice();
}

#endif
