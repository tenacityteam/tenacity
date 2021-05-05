/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.cpp
 
 Paul Licameli split from EffectManager.cpp
 
 **********************************************************************/


#include "RealtimeEffectManager.h"
#include "RealtimeEffectState.h"

// Tenacity libraries
#include <lib-components/EffectInterface.h>
#include <lib-project/Project.h>
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

bool RealtimeEffectManager::RealtimeIsActive() const noexcept
{
   return mStates.size() != 0;
}

bool RealtimeEffectManager::RealtimeIsSuspended() const noexcept
{
   return mSuspended;
}

void RealtimeEffectManager::RealtimeAddEffect(EffectProcessor &effect)
{
   // Block RealtimeProcess()
   SuspensionScope scope{ &mProject };

   // Add to list of active effects
   mStates.emplace_back( std::make_unique< RealtimeEffectState >( effect ) );
   auto &state = mStates.back();

   // Initialize effect if realtime is already active
   if (mActive)
   {
      // Initialize realtime processing
      effect.RealtimeInitialize();

      // Add the required processors
      for (size_t i = 0, cnt = mRealtimeChans.size(); i < cnt; i++)
      {
         state->RealtimeAddProcessor(i, mRealtimeChans[i], mRealtimeRates[i]);
      }
   }
}

void RealtimeEffectManager::RealtimeRemoveEffect(EffectProcessor &effect)
{
   // Block RealtimeProcess()
   SuspensionScope scope{ &mProject };

   if (mActive)
   {
      // Cleanup realtime processing
      effect.RealtimeFinalize();
   }
      
   // Remove from list of active effects
   auto end = mStates.end();
   auto found = std::find_if( mStates.begin(), end,
      [&](const decltype(mStates)::value_type &state){
         return &state->GetEffect() == &effect;
      }
   );
   if (found != end)
      mStates.erase(found);
}

void RealtimeEffectManager::RealtimeInitialize(double rate)
{
   // The audio thread should not be running yet, but protect anyway
   SuspensionScope scope{ &mProject };

   // (Re)Set processor parameters
   mRealtimeChans.clear();
   mRealtimeRates.clear();

   // RealtimeAdd/RemoveEffect() needs to know when we're active so it can
   // initialize newly added effects
   mActive = true;

   // Tell each effect to get ready for action
   for (auto &state : mStates) {
      state->GetEffect().SetSampleRate(rate);
      state->GetEffect().RealtimeInitialize();
   }
}

void RealtimeEffectManager::RealtimeAddProcessor(int group, unsigned chans, float rate)
{
   for (auto &state : mStates)
      state->RealtimeAddProcessor(group, chans, rate);

   mRealtimeChans.push_back(chans);
   mRealtimeRates.push_back(rate);
}

void RealtimeEffectManager::RealtimeFinalize()
{
   // Make sure nothing is going on
   RealtimeSuspend();

   // Tell each effect to clean up as well
   for (auto &state : mStates)
      state->GetEffect().RealtimeFinalize();

   // Reset processor parameters
   mRealtimeChans.clear();
   mRealtimeRates.clear();

   // No longer active
   mActive = false;
}

void RealtimeEffectManager::RealtimeSuspend()
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
   for (auto &state : mStates)
      state->RealtimeSuspend();
}

void RealtimeEffectManager::RealtimeSuspendOne( EffectProcessor &effect )
{
   auto begin = mStates.begin(), end = mStates.end();
   auto found = std::find_if( begin, end,
      [&effect]( const decltype( mStates )::value_type &state ){
         return state && &state->GetEffect() == &effect;
      }
   );
   if ( found != end )
      (*found)->RealtimeSuspend();
}

void RealtimeEffectManager::RealtimeResume() noexcept
{
   LockGuard lock(mLock);

   // Already running...bail
   if (!mSuspended)
   {
      return;
   }

   // Tell the effects to get ready for more action
   for (auto &state : mStates)
      state->RealtimeResume();

   // And we should too
   mSuspended = false;
}

void RealtimeEffectManager::RealtimeResumeOne( EffectProcessor &effect )
{
   auto begin = mStates.begin(), end = mStates.end();
   auto found = std::find_if( begin, end,
      [&effect]( const decltype( mStates )::value_type &state ){
         return state && &state->GetEffect() == &effect;
      }
   );
   if ( found != end )
      (*found)->RealtimeResume();
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::RealtimeProcessStart()
{
   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mSuspended)
   {
      for (auto &state : mStates)
      {
         if (state->IsRealtimeActive())
            state->GetEffect().RealtimeProcessStart();
      }
   }
}

//
// This will be called in a different thread than the main GUI thread.
//
size_t RealtimeEffectManager::RealtimeProcess(int group, unsigned chans, float **buffers, size_t numSamples)
{
   using namespace std::chrono;

   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended, so allow the samples to pass as-is.
   if (mSuspended || mStates.empty())
   {
      return numSamples;
   }

   // AK: If we have more channels than our input and output bufffers'
   // capacities, increase their capacities when necessary.
   if (mInputBuffers.capacity() < chans)
   {
      mInputBuffers.reserve(chans);
   } else if (mOutputBuffers.capacity() < chans)
   {
      mOutputBuffers.reserve(chans);
   }

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
   size_t called = 0;
   for (auto &state : mStates)
   {
      if (state->IsRealtimeActive())
      {
         state->RealtimeProcess(group, chans, mInputBuffers.data(),
                                mOutputBuffers.data(), numSamples
         );
         called++;
      }

      for (unsigned int j = 0; j < chans; j++)
      {
         std::swap(mInputBuffers[j], mOutputBuffers[j]);
      }
   }

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
void RealtimeEffectManager::RealtimeProcessEnd() noexcept
{
   // Protect ourselves from the main thread
   LockGuard lock(mLock);

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mSuspended)
   {
      for (auto &state : mStates)
      {
         if (state->IsRealtimeActive())
            state->GetEffect().RealtimeProcessEnd();
      }
   }
}

auto RealtimeEffectManager::GetRealtimeLatency() const -> Latency
{
   return mLatency;
}
