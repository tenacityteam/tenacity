/**********************************************************************

  Tenacity

  LyricsMain.cpp

  Avery King

  SPDX-License-Identifier: GPL-2.0-or-later

**********************************************************************/

#include "LyricsMain.h"

DEFINE_VERSION_CHECK

extern "C" DLL_API int ModuleDispatch(ModuleDispatchTypes type)
{
    switch (type)
    {
        case ModuleInitialize:
            break;
        default:
            break;
    }

    return 1;
}
