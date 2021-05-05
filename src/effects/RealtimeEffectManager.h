/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.h
 
 Paul Licameli split from EffectManager.h
 
 **********************************************************************/

#ifndef __AUDACITY_REALTIME_EFFECT_MANAGER__
#define __AUDACITY_REALTIME_EFFECT_MANAGER__

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>

#include "ClientData.h"

class TenacityProject;
class EffectProcessor;
class RealtimeEffectState;

class TENACITY_DLL_API RealtimeEffectManager final
   : public ClientData::Base
{
public:
   using Latency = std::chrono::microseconds;
   using EffectArray = std::vector <EffectProcessor*> ;

   RealtimeEffectManager(TenacityProject &project);
   ~RealtimeEffectManager();

   static RealtimeEffectManager & Get(TenacityProject &project);
   static const RealtimeEffectManager & Get(const TenacityProject &project);

   // Realtime effect processing
   bool RealtimeIsActive() const noexcept;
   bool RealtimeIsSuspended() const noexcept;
   void RealtimeAddEffect(EffectProcessor &effect);
   void RealtimeRemoveEffect(EffectProcessor &effect);
   void RealtimeInitialize(double rate);
   void RealtimeAddProcessor(int group, unsigned chans, float rate);
   void RealtimeFinalize();
   void RealtimeSuspend();
   void RealtimeSuspendOne( EffectProcessor &effect );
   void RealtimeResume() noexcept;
   void RealtimeResumeOne( EffectProcessor &effect );
   Latency GetRealtimeLatency() const;

   //! Object whose lifetime encompasses one suspension of processing in one thread
   class SuspensionScope {
   public:
      explicit SuspensionScope(TenacityProject *pProject)
         : mpProject{ pProject }
      {
         if (mpProject)
            Get(*mpProject).RealtimeSuspend();
      }
      SuspensionScope( SuspensionScope &&other )
         : mpProject{ other.mpProject }
      {
         other.mpProject = nullptr;
      }
      SuspensionScope& operator=( SuspensionScope &&other )
      {
         auto pProject = other.mpProject;
         other.mpProject = nullptr;
         mpProject = pProject;
         return *this;
      }
      ~SuspensionScope()
      {
         if (mpProject)
            Get(*mpProject).RealtimeResume();
      }

   private:
      TenacityProject *mpProject = nullptr;
   };

   //! Object whose lifetime encompasses one block of processing in one thread
   class ProcessScope {
   public:
      explicit ProcessScope(TenacityProject *pProject)
         : mpProject{ pProject }
      {
         if (mpProject)
            Get(*mpProject).RealtimeProcessStart();
      }
      ProcessScope( ProcessScope &&other )
         : mpProject{ other.mpProject }
      {
         other.mpProject = nullptr;
      }
      ProcessScope& operator=( ProcessScope &&other )
      {
         auto pProject = other.mpProject;
         other.mpProject = nullptr;
         mpProject = pProject;
         return *this;
      }
      ~ProcessScope()
      {
         if (mpProject)
            Get(*mpProject).RealtimeProcessEnd();
      }

      size_t Process( int group,
         unsigned chans, float **buffers, size_t numSamples)
      {
         if (mpProject)
            return Get(*mpProject)
               .RealtimeProcess(group, chans, buffers, numSamples);
         else
            return numSamples; // consider them trivially processed
      }

   private:
      TenacityProject *mpProject = nullptr;
   };

private:
   void RealtimeProcessStart();
   size_t RealtimeProcess(int group, unsigned chans, float **buffers, size_t numSamples);
   void RealtimeProcessEnd() noexcept;

   RealtimeEffectManager(const RealtimeEffectManager&) = delete;
   RealtimeEffectManager &operator=(const RealtimeEffectManager&) = delete;

   // Input and output buffers. Note that their capacity is equal to the number
   // of channels being processed.
   std::vector<float*> mInputBuffers;
   std::vector<float*> mOutputBuffers;

   TenacityProject &mProject;

   std::mutex mLock;
   std::vector< std::unique_ptr<RealtimeEffectState> > mStates;
   Latency mLatency{0};
   std::atomic_bool mSuspended;
   std::atomic_bool mActive;
   std::vector<unsigned> mRealtimeChans;
   std::vector<double> mRealtimeRates;
};

#endif
