/**********************************************************************

  Audacity: A Digital Audio Editor

  Benchmark.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_BENCHMARK__
#define __AUDACITY_BENCHMARK__

class wxWindow;
class TenacityProject;

SAUCEDACITY_DLL_API
void RunBenchmark( wxWindow *parent, TenacityProject &project );

#endif // define __AUDACITY_BENCHMARK__
