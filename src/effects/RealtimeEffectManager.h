/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.h
 
 Paul Licameli split from EffectManager.h
 
 **********************************************************************/

#ifndef __AUDACITY_REALTIME_EFFECT_MANAGER__
#define __AUDACITY_REALTIME_EFFECT_MANAGER__

#include <memory>
#include <mutex>
#include <vector>
#include <thread>

#include <boost/pool/pool.hpp>
using DefaultPoolAllocator = boost::default_user_allocator_new_delete;

class EffectClientInterface;
class RealtimeEffectState;

class SAUCEDACITY_DLL_API RealtimeEffectManager final
{
public:
   using EffectArray = std::vector <EffectClientInterface*> ;

   /** Get the singleton instance of the RealtimeEffectManager. **/
   static RealtimeEffectManager & Get();

   // Realtime effect processing
   bool RealtimeIsActive();
   bool RealtimeIsSuspended();
   void RealtimeAddEffect(EffectClientInterface *effect);
   void RealtimeRemoveEffect(EffectClientInterface *effect);
   void RealtimeSetEffects(const EffectArray & mActive);
   void RealtimeInitialize(double rate);
   void RealtimeAddProcessor(int group, unsigned chans, float rate);
   void RealtimeFinalize();
   void RealtimeSuspend();
   void RealtimeSuspendOne( EffectClientInterface &effect );
   void RealtimeResume();
   void RealtimeResumeOne( EffectClientInterface &effect );
   void RealtimeProcessStart();
   size_t RealtimeProcess(int group, unsigned chans, float **buffers, size_t numSamples);
   void RealtimeProcessEnd();
   std::chrono::milliseconds GetRealtimeLatency();

private:
   RealtimeEffectManager();
   ~RealtimeEffectManager();

   boost::pool<DefaultPoolAllocator> mMemoryPool;

   // Input and output buffers. Note that their capacity is equal to the number
   // of channels being processed.
   std::vector<float*> mInputBuffers;
   std::vector<float*> mOutputBuffers;

   std::mutex mLock;
   std::vector< std::unique_ptr<RealtimeEffectState> > mStates;
   std::chrono::milliseconds mRealtimeLatency;
   bool mRealtimeSuspended;
   bool mRealtimeActive;
   std::vector<unsigned> mRealtimeChans;
   std::vector<double> mRealtimeRates;
};

#endif
