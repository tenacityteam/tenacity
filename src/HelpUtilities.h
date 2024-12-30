/**********************************************************************

Audacity: A Digital Audio Editor

@file HelpUtilities.cpp

Paul Licameli split from HelpMenus.cpp

**********************************************************************/

#ifndef __AUDACITY_HELP_UTILITIES__
#define __AUDACITY_HELP_UTILITIES__

class AudacityProject;
class wxString;
class TranslatableString;

void TENACITY_DLL_API ShowDiagnostics(
   AudacityProject &project, const wxString &info,
   const TranslatableString &description, const wxString &defaultPath,
   bool fixedWidth = false);

#endif
