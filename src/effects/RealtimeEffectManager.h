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
class Track;

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
   bool IsActive() const noexcept;
   void Initialize(double rate);
   void AddTrack(int group, unsigned chans, float rate);
   void Finalize();
   void Suspend();
   void Resume() noexcept;
   Latency GetLatency() const;

   //! Object whose lifetime encompasses one suspension of processing in one thread
   class SuspensionScope {
   public:
      explicit SuspensionScope(TenacityProject *pProject)
         : mpProject{ pProject }
      {
         if (mpProject)
            Get(*mpProject).Suspend();
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
            Get(*mpProject).Resume();
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
            Get(*mpProject).ProcessStart();
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
            Get(*mpProject).ProcessEnd();
      }

      size_t Process( int group,
         unsigned chans, float **buffers, size_t numSamples)
      {
         if (mpProject)
            return Get(*mpProject)
               .Process(group, chans, buffers, numSamples);
         else
            return numSamples; // consider them trivially processed
      }

   private:
      TenacityProject *mpProject = nullptr;
   };

private:
   void ProcessStart();
   size_t Process(int group, unsigned chans, float **buffers, size_t numSamples);
   void ProcessEnd() noexcept;

   RealtimeEffectManager(const RealtimeEffectManager&) = delete;
   RealtimeEffectManager &operator=(const RealtimeEffectManager&) = delete;

   using StateVisitor =
      std::function<void(RealtimeEffectState &state, bool bypassed)> ;

   //! Visit the per-project states first, then states for leader if not null
   void VisitGroup(Track *leader, StateVisitor func);

   TenacityProject& mProject;

   // Input and output buffers. Note that their capacity is equal to the number
   // of channels being processed.
   std::vector<float*> mInputBuffers;
   std::vector<float*> mOutputBuffers;

   std::mutex mLock;
   Latency mLatency{0};
   std::atomic<bool> mSuspended{ true };
   std::atomic<bool> mActive{ false };
   std::vector<unsigned> mChans;
   std::vector<double> mRates;
};

#endif
