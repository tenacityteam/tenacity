/**********************************************************************

  Tenacity

  DynamicCompressorBase.h

  Max Maisel (based on Compressor effect)
  Avery King (split from the original Compressor2.h)

**********************************************************************/

#pragma once

#include "Effect.h"
#include "EffectInterface.h"
#include "Envelope.h"
#include "SampleCount.h"
#include "SampleFormat.h"
#include "SettingsVisitor.h"
#include "StatefulEffect.h"
#include "WaveTrack.h"

#include <vector>

//// Utility Classes //////////////////////////////////////////////////////////

class SamplePreprocessor
{
    public:
        virtual ~SamplePreprocessor() = default;
        virtual float ProcessSample(float value) = 0;
        virtual float ProcessSample(float valueL, float valueR) = 0;
        virtual void Reset(float value = 0) = 0;
        virtual void SetWindowSize(size_t windowSize) = 0;
};

class SlidingRmsPreprocessor : public SamplePreprocessor
{
    public:
        SlidingRmsPreprocessor(size_t windowSize, float gain = 2.0);

        virtual float ProcessSample(float value);
        virtual float ProcessSample(float valueL, float valueR);
        virtual void Reset(float value = 0);
        virtual void SetWindowSize(size_t windowSize);

        static const size_t REFRESH_WINDOW_EVERY = 1048576; // 1 MB

    private:
        float mSum;
        float mGain;
        std::vector<float> mWindow;
        size_t mPos;
        size_t mInsertCount;

        inline float DoProcessSample(float value);
        void Refresh();
};

class SlidingMaxPreprocessor : public SamplePreprocessor
{
    public:
        SlidingMaxPreprocessor(size_t windowSize);

        virtual float ProcessSample(float value);
        virtual float ProcessSample(float valueL, float valueR);
        virtual void Reset(float value = 0);
        virtual void SetWindowSize(size_t windowSize);

    private:
        std::vector<float> mWindow;
        std::vector<float> mMaxes;
        size_t mPos;

        inline float DoProcessSample(float value);
};

class EnvelopeDetector
{
    public:
        EnvelopeDetector(size_t buffer_size);
        virtual ~EnvelopeDetector() = default;

        float ProcessSample(float value);
        size_t GetBlockSize() const;
        const float* GetBuffer(int idx) const;

        virtual void CalcInitialCondition(float value);
        inline float InitialCondition() const { return mInitialCondition; }
        inline size_t InitialConditionSize() const { return mInitialBlockSize; }

        virtual void Reset(float value = 0) = 0;
        virtual void SetParams(float sampleRate, float attackTime,
            float releaseTime) = 0;

        virtual float AttackFactor();
        virtual float DecayFactor();

    protected:
        size_t mPos;
        float mInitialCondition;
        size_t mInitialBlockSize;
        std::vector<float> mLookaheadBuffer;
        std::vector<float> mProcessingBuffer;
        std::vector<float> mProcessedBuffer;

        virtual void Follow() = 0;
};

class ExpFitEnvelopeDetector : public EnvelopeDetector
{
    public:
        ExpFitEnvelopeDetector(float rate, float attackTime, float releaseTime,
            size_t buffer_size);

        virtual void Reset(float value);
        virtual void SetParams(float sampleRate, float attackTime,
            float releaseTime);

    private:
        double mAttackFactor;
        double mReleaseFactor;

        virtual void Follow();
};

class Pt1EnvelopeDetector : public EnvelopeDetector
{
    public:
        Pt1EnvelopeDetector(float rate, float attackTime, float releaseTime,
            size_t buffer_size, bool correctGain = true);
        virtual void CalcInitialCondition(float value);

        virtual void Reset(float value);
        virtual void SetParams(float sampleRate, float attackTime,
            float releaseTime);
        virtual float AttackFactor();
        virtual float DecayFactor();

    private:
        bool mCorrectGain;
        double mGainCorrection;
        double mAttackFactor;
        double mReleaseFactor;

        virtual void Follow();
};

class PipelineBuffer
{
    public:
        sampleCount trackPos;
        size_t trackSize;
        size_t size;

        inline float* operator[](size_t idx)
            { return mBlockBuffer[idx].get(); }

        void pad_to(size_t len, float value, bool stereo);
        void swap(PipelineBuffer& other);
        void init(size_t size, bool stereo);
        void fill(float value, bool stereo);
        inline size_t capacity() const { return mCapacity; }
        void free();

    private:
        size_t mCapacity;
        Floats mBlockBuffer[2];
};

///////////////////////////////////////////////////////////////////////////////

enum kAlgorithms
{
    kExpFit,
    kEnvPT1,
    nAlgos
};

enum kCompressBy
{
    kAmplitude,
    kRMS,
    nBy
};


/** @brief The actual logic powering Tenacity's Dynamic Compressor effect.
 *
 * This class is based on the old EffectCompressor2 class. The implementation
 * itself has been minimally modified to work with the newer effect interfaces.
 *
 * This class is not to be confused with any DynamicRange* classes, as they are
 * completely unrelated to this effect right here. None of them are used by
 * this class either.
 */
class BUILTIN_EFFECTS_API DynamicCompressorBase : public StatefulEffect
{
    public:
        static inline DynamicCompressorBase* FetchParameters(DynamicCompressorBase& e, EffectSettings&)
        {
            return &e;
        }

        static const ComponentInterfaceSymbol Symbol;

        DynamicCompressorBase();
        virtual ~DynamicCompressorBase() override = default;

        // ComponentInterface implementation

        ComponentInterfaceSymbol GetSymbol() const override;
        TranslatableString GetDescription() const override;
        ManualPageID ManualPage() const override;

        // EffectDefinitionInterface implementation

        EffectType GetType() const override;
        RealtimeSince RealtimeSupport() const override;

        // StatefulEffectBase implementation

        unsigned GetAudioInCount() const override;
        unsigned GetAudioOutCount() const override;

        // Member functions for realtime usage. Note: currently unused.
        bool RealtimeInitialize(EffectSettings& settings, double sampleRate) override;
        bool RealtimeAddProcessor(
            EffectSettings& settings, EffectOutputs* pOutputs,
            unsigned numChannels, float sampleRate
        ) override;
        bool RealtimeFinalize(EffectSettings& settings) noexcept override;
        size_t RealtimeProcess(
            size_t group, EffectSettings& settings, const float* const* inBuf,
            float* const* outBuf, size_t numSamples) override;

        // Effect implementation
        bool Process(EffectInstance &instance, EffectSettings &settings) override;
        RegistryPaths GetFactoryPresets() const override;
        OptionalMessage LoadFactoryPreset(int id, EffectSettings& settings) const override;

    protected:
        // FIXME: Get rid of this member function when presets are externalized
        // to somewhere else.
        OptionalMessage DoLoadFactoryPreset(int id);

        // DynamicCompressorBase implementation
        double CompressorGain(double env);
        std::unique_ptr<SamplePreprocessor> InitPreprocessor(
            double rate, bool preview = false);
        std::unique_ptr<EnvelopeDetector> InitEnvelope(
            double rate, size_t blockSize = 0, bool preview = false);
        size_t CalcBufferSize(double sampleRate);

        size_t CalcLookaheadLength(double rate);
        inline size_t CalcWindowLength(double rate);

        void AllocPipeline();
        void AllocRealtimePipeline(double sampleRate);
        void FreePipeline();
        void SwapPipeline();
        bool ProcessOne(TrackIterRange<WaveTrack> range);
        bool LoadPipeline(TrackIterRange<WaveTrack> range, size_t len);
        void FillPipeline();
        void ProcessPipeline();
        inline float PreprocSample(PipelineBuffer& pbuf, size_t rp);
        inline float EnvelopeSample(PipelineBuffer& pbuf, size_t rp);
        inline void CompressSample(float env, size_t wp);
        bool PipelineHasData();
        void DrainPipeline();
        void StorePipeline(TrackIterRange<WaveTrack> range);

        bool UpdateProgress();
        void UpdateRealtimeParams(double sampleRate);

        static const int TAU_FACTOR = 5;
        static const size_t MIN_BUFFER_CAPACITY = 1048576; // 1MB

        static const size_t PIPELINE_DEPTH = 4;
        PipelineBuffer mPipeline[PIPELINE_DEPTH];

        double mCurT0;
        double mCurT1;
        double mProgressVal;
        double mTrackLen;
        bool mProcStereo;

        std::mutex mRealtimeMutex;
        std::unique_ptr<SamplePreprocessor> mPreproc;
        std::unique_ptr<EnvelopeDetector> mEnvelope;

        // Effect parameters
        const EffectParameterMethods& Parameters() const override;

        int    mAlgorithm;
        int    mCompressBy;
        bool   mStereoInd;

        double    mThresholdDB;
        double    mRatio;
        double    mKneeWidthDB;
        double    mAttackTime;
        double    mReleaseTime;
        double    mLookaheadTime;
        double    mLookbehindTime;
        double    mOutputGainDB;

        static const EnumValueSymbol kAlgorithmsStrings[nAlgos];
        static const EnumValueSymbol kCompressByStrings[nBy];

        static constexpr EnumParameter Algorithm {
            &DynamicCompressorBase::mAlgorithm,
            L"Algorithm",
            kEnvPT1,
            0,
            nAlgos - 1,
            1,
            kAlgorithmsStrings,
            nAlgos
        };

        static constexpr EnumParameter CompressBy {
            &DynamicCompressorBase::mCompressBy,
            L"CompressBy",
            kAmplitude,
            0,
            nBy - 1,
            1,
            kCompressByStrings,
            nBy
        };

        static constexpr EffectParameter StereoInd {
            &DynamicCompressorBase::mStereoInd,
            L"StereoIndependent",
            false,
            false,
            true,
            1
        };

        static constexpr EffectParameter Threshold {
            &DynamicCompressorBase::mThresholdDB,
            L"Threshold",
            -12.0,
            -60.0,
            -1.0,
            1.0
        };

        static constexpr EffectParameter Ratio {
            &DynamicCompressorBase::mRatio,
            L"Ratio",
            2.0,
            1.1,
            100.0,
            20.0
        };

        static constexpr EffectParameter KneeWidth {
            &DynamicCompressorBase::mKneeWidthDB,
            L"KneeWidth",
            10.0,
            0.0,
            20.0,
            10.0
        };

        static constexpr EffectParameter AttackTime {
            &DynamicCompressorBase::mAttackTime,
            L"AttackTime",
            0.2,
            0.0001,
            30.0,
            2000.0
        };

        static constexpr EffectParameter ReleaseTime {
            &DynamicCompressorBase::mReleaseTime,
            L"ReleaseTime",
            1.0,
            0.0001,
            30.0,
            2000.0
        };

        static constexpr EffectParameter LookaheadTime {
            &DynamicCompressorBase::mLookaheadTime,
            L"LookaheadTime",
            0.0,
            0.0,
            10.0,
            200.0
        };

        static constexpr EffectParameter LookbehindTime {
            &DynamicCompressorBase::mLookbehindTime,
            L"LookbehindTime",
            0.0,
            0.0,
            10.0,
            200.0
        };

        static constexpr EffectParameter OutputGain {
            &DynamicCompressorBase::mOutputGainDB,
            L"OutputGain",
            0.0,
            0.0,
            50.0,
            10.0
        };

        // cached intermediate values
        size_t mLookaheadLength;

        static const size_t RESPONSE_PLOT_SAMPLES = 200;
        static const size_t RESPONSE_PLOT_TIME = 5;
        static const size_t RESPONSE_PLOT_STEP_START = 2;
        static const size_t RESPONSE_PLOT_STEP_STOP = 3;
};
