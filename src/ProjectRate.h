/*!********************************************************************

Audacity: A Digital Audio Editor

@file ProjectRate.h
@brief an object holding per-project preferred sample rate

Paul Licameli split from ProjectSettings.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_RATE__
#define __AUDACITY_PROJECT_RATE__

class TenacityProject;

#include "ClientData.h"
#include <wx/event.h> // to declare custom event type

// Sent to the project when the rate changes
wxDECLARE_EXPORTED_EVENT(TENACITY_DLL_API,
   EVT_PROJECT_RATE_CHANGE, wxEvent);

///\brief Holds project sample rate
class TENACITY_DLL_API ProjectRate final
   : public ClientData::Base
{
public:
   static ProjectRate &Get( TenacityProject &project );
   static const ProjectRate &Get( const TenacityProject &project );
   
   explicit ProjectRate(TenacityProject &project);
   ProjectRate( const ProjectRate & ) = delete;
   ProjectRate &operator=( const ProjectRate & ) = delete;

   void SetRate(double rate);
   double GetRate() const;

private:
   TenacityProject &mProject;
   double mRate;
};

#endif

