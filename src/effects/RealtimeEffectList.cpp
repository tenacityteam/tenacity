/**********************************************************************
 
  Audacity: A Digital Audio Editor
 
  RealtimeEffectList.cpp
 
 *********************************************************************/

#include "RealtimeEffectList.h"

#include "Project.h"
#include "Track.h"

RealtimeEffectList::RealtimeEffectList()
{
}

RealtimeEffectList::~RealtimeEffectList()
{
}

static const AttachedProjectObjects::RegisteredFactory masterEffects
{
   [](TenacityProject &project)
   {
      return std::make_shared<RealtimeEffectList>();
   }
};

RealtimeEffectList &RealtimeEffectList::Get(TenacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectList>(masterEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const TenacityProject &project)
{
   return Get(const_cast<TenacityProject &>(project));
}

static const AttachedTrackObjects::RegisteredFactory trackEffects
{
   [](Track &track)
   {
      return std::make_shared<RealtimeEffectList>();
   }
};

RealtimeEffectList &RealtimeEffectList::Get(Track &track)
{
   return track.AttachedObjects::Get<RealtimeEffectList>(trackEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const Track &track)
{
   return Get(const_cast<Track &>(track));
}
