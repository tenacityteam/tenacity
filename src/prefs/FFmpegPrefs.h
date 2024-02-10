/**********************************************************************

  Tenacity

  FFmpegPrefs.h

  Joshua Haberman
  Dominic Mazzoni
  James Crook

**********************************************************************/

#pragma once

#include <wx/defs.h>

#include "PrefsPanel.h"

class wxStaticText;
class wxTextCtrl;
class ReadOnlyText;
class ShuttleGui;

#define FFMPEG_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("FFmpeg") }

class FFmpegPrefs final : public PrefsPanel
{
 public:
   FFmpegPrefs(wxWindow * parent, wxWindowID winid);
   ~FFmpegPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
   void Populate();
   void SetFFmpegVersionText();

   void OnFFmpegFindButton(wxCommandEvent & e);

   ReadOnlyText *mFFmpegVersion;
};
