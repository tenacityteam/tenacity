/**********************************************************************

Audacity: A Digital Audio Editor

@file SaucedacityFileConfig.h
@brief Extend FileConfig with application-specific behavior

Paul Licameli split from Prefs.h

**********************************************************************/

#ifndef __AUDACITY_FILE_CONFIG__
#define __AUDACITY_FILE_CONFIG__

#include <memory>

// Saucedacity libraries
#include <lib-preferences/FileConfig.h> // to inherit

/// \brief Our own specialisation of FileConfig.
class SAUCEDACITY_DLL_API SaucedacityFileConfig final : public FileConfig
{
public:
   //! Require a call to this factory, to guarantee proper two-phase initialization
   static std::unique_ptr<SaucedacityFileConfig> Create(
      const wxString& appName = {},
      const wxString& vendorName = {},
      const wxString& localFilename = {},
      const wxString& globalFilename = {},
      long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
      const wxMBConv& conv = wxConvAuto()
   );

   ~SaucedacityFileConfig() override;

protected:
   void Warn() override;

private:
   //! Disallow direct constructor call, because a two-phase initialization is required
   SaucedacityFileConfig(
      const wxString& appName,
      const wxString& vendorName,
      const wxString& localFilename,
      const wxString& globalFilename,
      long style,
      const wxMBConv& conv
   );
};
#endif
