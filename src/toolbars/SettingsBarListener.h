/**********************************************************************

  Tenacity

  SettingsBarListener.h

  Avery King split form SelectionBarListener.h
  Dominic Mazzoni

**********************************************************************/

#pragma once

// Tenacity libraries
#include <lib-components/ComponentInterfaceSymbol.h>

/// A parent class of SettingsBar, used to forward events to do with changes in
/// the SettingsBar.
class TENACITY_DLL_API SettingsBarListener // not final
{
    public:
        virtual ~SettingsBarListener() = default;

        virtual double AS_GetRate() = 0;
        virtual void AS_SetRate(double rate) = 0;
        virtual int AS_GetSnapTo() = 0;
        virtual void AS_SetSnapTo(int snap) = 0;
};

// class TENACITY_DLL_API TimeToolBarListener // not final
// {
//     public:
//         TimeToolBarListener() = default;
//         virtual ~TimeToolBarListener();

//         virtual const NumericFormatSymbol & TT_GetAudioTimeFormat() = 0;
//         virtual void TT_SetAudioTimeFormat(const NumericFormatSymbol & format) = 0;
// };
