/**********************************************************************

  Audacity: A Digital Audio Editor

  HelpText.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_HELP_TEXT__
#define __AUDACITY_HELP_TEXT__

class TranslatableString;
class wxString;

// Tenacity libraries
#include <lib-strings/Identifier.h>

struct URLStringTag;
//! Distinct type for URLs
using URLString = TaggedIdentifier< URLStringTag >;

TENACITY_DLL_API  wxString HelpText( const wxString Key );
TENACITY_DLL_API TranslatableString TitleText( const wxString & Key );

extern TENACITY_DLL_API const wxString VerCheckHtml();
extern TENACITY_DLL_API wxString FormatHtmlText( const wxString & Text );

#endif
