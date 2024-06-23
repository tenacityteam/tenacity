/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.cpp
 
 Paul Licameli split from EffectManager.cpp
 
 **********************************************************************/


#include "RealtimeEffectManager.h"
#include "RealtimeEffectList.h"
#include "RealtimeEffectState.h"

// Tenacity libraries
#include <lib-project/Project.h>
#include <lib-track/Track.h>
#include <lib-utility/MemoryX.h>

#include <atomic>
#include <memory>
#include <chrono>

using LockGuard = std::lock_guard<std::mutex>;

static const AttachedProjectObjects::RegisteredFactory manager
{
   [](TenacityProject &project)
   {
      return std::make_shared<RealtimeEffectManager>(project);
   }
};

RealtimeEffectManager &RealtimeEffectManager::Get(TenacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectManager&>(manager);
}

const RealtimeEffectManager &RealtimeEffectManager::Get(const TenacityProject &project)
{
   return Get(const_cast<TenacityProject &>(project));
}

RealtimeEffectManager::RealtimeEffectManager(TenacityProject &project)
   : mProject(project)
{
   // Allocate our vectors. We set their capacity to a size of '2', enough to
   // process stero audio data.
   mInputBuffers.reserve(2);
   mOutputBuffers.reserve(2);
}

RealtimeEffectManager::~RealtimeEffectManager()
{
}

bool RealtimeEffectManager::IsActive() const noexcept
{
   return mActive;
}

void RealtimeEffectManager::Initialize(double rate)
{
   // Remember the rate
   mRate = rate;

   // (Re)Set processor parameters
   mChans.clear();
   mRates.clear();
   mGroupLeaders.clear();

   // RealtimeAdd/RemoveEffect() needs to know when we're active so it can
   // initialize newly added effects
   mActive = true;

   // Tell each effect of the master list to get ready for action
   VisitGroup(nullptr, [rate](RealtimeEffectState &state, bool){
      state.Initialize(rate);
   });

   // Leave suspended state
   Resume();
}

void RealtimeEffectManager::AddTrack(Track *track, unsigned chans, float rate)
{
   auto leader = *track->GetOwner()->FindLeader(track);
   mGroupLeaders.push_back(leader);
   mChans.insert({leader, chans});
   mRates.insert({leader, rate});

   VisitGroup(leader,
      [&](RealtimeEffectState & state, bool) {
         state.AddTrack(leader, chans, rate);
      }
   );
}

void RealtimeEffectManager::Finalize() noexcept
{
   // Reenter suspended state
   Suspend();

   // Assume it is now safe to clean up
   mLatency = std::chrono::microseconds(0);

   VisitAll([](auto &state, bool){ state.Finalize(); });

   // Reset processor parameters
   mGroupLeaders.clear();
   mChans.clear();
   mRates.clear();

   // No longer active
   mActive = false;
}

void RealtimeEffectManager::Suspend()
{
   // Already suspended...bail
   if (mSuspended)
   {
      return;
   }

   LockGuard lock(mLock);

   // Show that we aren't going to be doing anything
   mSuspended = true;

   // And make sure the effects don't either
   VisitGroup(nullptr, [](RealtimeEffectState &state, bool){
      state.Suspend();
   });
}

void RealtimeEffectManager::Resume() noexcept
{
   LockGuard lock(mLock);

   // Already running...bail
   if (!mSuspended)
   {
      return;
   }

   // Tell the effects to get ready for more action
   VisitGroup(nullptr, [](RealtimeEffectState &state, bool){
      state.Resume();
   });

   // And we should too
   mSuspended = false;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessStart()
{
   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mSuspended)
   {
      VisitGroup(nullptr, [](RealtimeEffectState &state, bool bypassed){
         if (!bypassed)
            state.ProcessStart();
      });
   }
}

//
// This will be called in a different thread than the main GUI thread.
//
size_t RealtimeEffectManager::Process(Track *track,
                                      float **buffers,
                                      size_t numSamples)
{
   using namespace std::chrono;

   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended, so allow the samples to pass as-is.
   if (mSuspended)
      return numSamples;

   auto chans = mChans[track];

   // AK: If we have more channels than our input and output bufffers'
   // capacities, increase their capacities when necessary.
   if (mInputBuffers.capacity() < chans)
   {
      mInputBuffers.reserve(chans);
   } else if (mOutputBuffers.capacity() < chans)
   {
      mOutputBuffers.reserve(chans);
   }

   mInputBuffers.resize(chans);
   mOutputBuffers.resize(chans);

   // Remember when we started so we can calculate the amount of latency we
   // are introducing
   auto start = steady_clock::now();

   // Allocate the in/out buffer arrays
   // GP: temporary fix until we convert Effect
   AutoAllocator<float>  floatAllocator;

   // And populate the input with the buffers we've been given while allocating
   // NEW output buffers
   for (unsigned int i = 0; i < chans; i++)
   {
      mInputBuffers[i] = buffers[i];
      mOutputBuffers[i] = floatAllocator.Allocate(true, numSamples);
   }

   // Now call each effect in the chain while swapping buffer pointers to feed the
   // output of one effect as the input to the next effect
   // Tracks how many processors were called
   size_t called = 0;
   VisitGroup(track,
      [&](RealtimeEffectState &state, bool bypassed)
      {
         if (bypassed)
            return;

         state.Process(track, chans, mInputBuffers.data(),
                       mOutputBuffers.data(), numSamples);
         for (auto i = 0; i < chans; ++i)
            std::swap(mInputBuffers[i], mOutputBuffers[i]);
         called++;
      }
   );

   // Once we're done, we might wind up with the last effect storing its results
   // in the temporary buffers.  If that's the case, we need to copy it over to
   // the caller's buffers.  This happens when the number of effects processed
   // is odd.
   if (called & 1)
   {
      for (unsigned int i = 0; i < chans; i++)
      {
         memcpy(buffers[i], mInputBuffers[i], numSamples * sizeof(float));
      }
   }

   // Remember the latency
   mLatency = duration_cast<std::chrono::microseconds>(steady_clock::now() - start);

   //
   // This is wrong...needs to handle tails
   //
   return numSamples;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessEnd() noexcept
{
   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mSuspended)
   {
      VisitGroup(nullptr, [](RealtimeEffectState &state, bool bypassed){
         if (!bypassed)
            state.ProcessEnd();
      });
   }
}

void RealtimeEffectManager::VisitGroup(Track *leader, StateVisitor func)
{
   // Call the function for each effect on the master list
   RealtimeEffectList::Get(mProject).Visit(func);

   // Call the function for each effect on the track list
   if (leader)
     RealtimeEffectList::Get(*leader).Visit(func);
}

void RealtimeEffectManager::VisitAll(StateVisitor func)
{
   // Call the function for each effect on the master list
   RealtimeEffectList::Get(mProject).Visit(func);

   // And all track lists
   for (auto leader : mGroupLeaders)
      RealtimeEffectList::Get(*leader).Visit(func);
}

RealtimeEffectState *
RealtimeEffectManager::AddState(
   RealtimeEffects::InitializationScope *pScope,
   Track *pTrack, const PluginID & id)
{
   auto pLeader = pTrack ? *TrackList::Channels(pTrack).begin() : nullptr;
   RealtimeEffectList &states = pLeader
      ? RealtimeEffectList::Get(*pLeader)
      : RealtimeEffectList::Get(mProject);

   std::optional<RealtimeEffects::SuspensionScope> myScope;
   if (mActive) {
      if (pScope)
         myScope.emplace(*pScope, mProject.weak_from_this());
      else
         return nullptr;
   }
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   auto pState = states.AddState(id);
   if (!pState)
      return nullptr;
   auto &state = *pState;
   
   if (mActive)
   {
      // Adding a state while playback is in-flight
      state.Initialize(mRate);

      for (auto &leader : mGroupLeaders) {
         // Add all tracks to a per-project state, but add only the same track
         // to a state in the per-track list
         if (pLeader && pLeader != leader)
            continue;

         auto chans = mChans[leader];
         auto rate = mRates[leader];

         state.AddTrack(leader, chans, rate);
      }
   }
   return &state;
}

void RealtimeEffectManager::RemoveState(
   RealtimeEffects::InitializationScope *pScope,
   Track *pTrack, RealtimeEffectState &state)
{
   auto pLeader = pTrack ? *TrackList::Channels(pTrack).begin() : nullptr;
   RealtimeEffectList &states = pLeader
      ? RealtimeEffectList::Get(*pLeader)
      : RealtimeEffectList::Get(mProject);

   std::optional<RealtimeEffects::SuspensionScope> myScope;
   if (mActive) {
      if (pScope)
         myScope.emplace(*pScope, mProject.weak_from_this());
      else
         return;
   }
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   if (mActive)
      state.Finalize();

   states.RemoveState(state);
}

auto RealtimeEffectManager::GetLatency() const -> Latency
{
   return mLatency;
}
