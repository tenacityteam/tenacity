/**********************************************************************

  Tenacity

  DynamicCompressorBase.h

  Max Maisel (based on Compressor effect)
  Avery King (split from the original Compressor2.h)

**********************************************************************/

#pragma once

#include "SampleCount.h"
#include "SampleFormat.h"

#include <vector>

//// Utility Classes //////////////////////////////////////////////////////////

class SamplePreprocessor
{
    public:
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
