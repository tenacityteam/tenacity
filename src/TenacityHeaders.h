/**********************************************************************

  Audacity: A Digital Audio Editor

  TenacityHeaders.h

  Dominic Mazzoni
**********************************************************************/



#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#ifdef __WXMSW__
#include <initializer_list>
#endif

#include <wx/wx.h>
#include <wx/bitmap.h>
#include <wx/filefn.h>
#include <wx/image.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/tooltip.h>

// Tenacity libraries
#include <lib-math/FFT.h>
#include <lib-preferences/Prefs.h>
#include <lib-strings/Identifier.h>

#include "PaintManager.h"
#include "AudioIO.h"
#include "Diags.h"
#include "Envelope.h"
#include "FileFormats.h"
#include "ImageManipulation.h"
#include "LabelTrack.h"
#include "Mix.h"
#include "NoteTrack.h"
#include "Sequence.h"
#include "TimeTrack.h"
#include "UndoManager.h"
#include "WaveTrack.h"
#include "widgets/ASlider.h"
#include "widgets/ProgressDialog.h"
#include "widgets/Ruler.h"

// PRL:  These lines allow you to remove Project.h above.
// They must be included before the definition of macro NEW below.
#include <set>
#include <map>

//#ifdef __WXMSW__
// Enable this to diagnose memory leaks too!
//    #include <wx/msw/msvcrt.h>      // redefines the NEW() operator
//#endif

#ifdef _MSC_VER
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#undef new
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
