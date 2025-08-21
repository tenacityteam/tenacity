/**********************************************************************

  Tenacity

  DynamicCompressorBase.cpp

  Max Maisel (based on Compressor effect)

**********************************************************************/

#include "DynamicCompressorBase.h"

#include <cassert>

//// SlidingRmsPreprocessor Implementation ////////////////////////////////////
SlidingRmsPreprocessor::SlidingRmsPreprocessor(size_t windowSize, float gain)
    : mSum(0),
    mGain(gain),
    mWindow(windowSize, 0),
    mPos(0),
    mInsertCount(0)
{
}

float SlidingRmsPreprocessor::ProcessSample(float value)
{
    return DoProcessSample(value * value);
}

float SlidingRmsPreprocessor::ProcessSample(float valueL, float valueR)
{
    return DoProcessSample((valueL * valueL + valueR * valueR) / 2.0);
}

void SlidingRmsPreprocessor::Reset(float level)
{
    mSum = (level / mGain) * (level / mGain) * float(mWindow.size());
    mPos = 0;
    mInsertCount = 0;
    std::fill(mWindow.begin(), mWindow.end(), 0);
}

void SlidingRmsPreprocessor::SetWindowSize(size_t windowSize)
{
    mWindow.resize(windowSize);
    Reset();
}

float SlidingRmsPreprocessor::DoProcessSample(float value)
{
    if(mInsertCount > REFRESH_WINDOW_EVERY)
    {
        // Update RMS sum directly from the circle buffer every
        // REFRESH_WINDOW_EVERY samples to avoid accumulation of rounding errors.
        mWindow[mPos] = value;
        Refresh();
    }
    else
    {
        // Calculate current level from root-mean-squared of
        // circular buffer ("RMS").
        mSum -= mWindow[mPos];
        mWindow[mPos] = value;
        mSum += mWindow[mPos];
        ++mInsertCount;
    }

    // Also refresh if there are severe rounding errors that
    // caused mRMSSum to be negative.
    if(mSum < 0)
        Refresh();

    mPos = (mPos + 1) % mWindow.size();

    // Multiply by gain (usually two) to approximately correct peak level
    // of standard audio (avoid clipping).
    return mGain * sqrt(mSum/float(mWindow.size()));
}

void SlidingRmsPreprocessor::Refresh()
{
    // Recompute the RMS sum periodically to prevent accumulation
    // of rounding errors during long waveforms.
    mSum = 0;
    for(const auto& sample : mWindow)
        mSum += sample;
    mInsertCount = 0;
}

//// SlidingMaxPreprocessor Implementation ////////////////////////////////////

SlidingMaxPreprocessor::SlidingMaxPreprocessor(size_t windowSize)
    : mWindow(windowSize, 0),
    mMaxes(windowSize, 0),
    mPos(0)
{
}

float SlidingMaxPreprocessor::ProcessSample(float value)
{
    return DoProcessSample(fabs(value));
}

float SlidingMaxPreprocessor::ProcessSample(float valueL, float valueR)
{
    return DoProcessSample((fabs(valueL) + fabs(valueR)) / 2.0);
}

void SlidingMaxPreprocessor::Reset(float value)
{
    mPos = 0;
    std::fill(mWindow.begin(), mWindow.end(), value);
    std::fill(mMaxes.begin(), mMaxes.end(), value);
}

void SlidingMaxPreprocessor::SetWindowSize(size_t windowSize)
{
    mWindow.resize(windowSize);
    mMaxes.resize(windowSize);
    Reset();
}

float SlidingMaxPreprocessor::DoProcessSample(float value)
{
    size_t oldHead     = (mPos-1) % mWindow.size();
    size_t currentHead = mPos;
    size_t nextHead    = (mPos+1) % mWindow.size();
    mWindow[mPos] = value;
    mMaxes[mPos]  = std::max(value, mMaxes[oldHead]);

    if(mPos % ((mWindow.size()+1)/2) == 0)
    {
        mMaxes[mPos] = mWindow[mPos];
        for(size_t i = 1; i < mWindow.size(); ++i)
        {
            size_t pos1 = (mPos-i+mWindow.size()) % mWindow.size();
            size_t pos2 = (mPos-i+mWindow.size()+1) % mWindow.size();
            mMaxes[pos1] = std::max(mWindow[pos1], mMaxes[pos2]);
        }
    }
    mPos = nextHead;
    return std::max(mMaxes[currentHead], mMaxes[nextHead]);
}

//// EnvelopeDetector implementation //////////////////////////////////////////

EnvelopeDetector::EnvelopeDetector(size_t buffer_size)
    : mPos(0),
    mInitialCondition(0),
    mInitialBlockSize(0),
    mLookaheadBuffer(buffer_size, 0),
    mProcessingBuffer(buffer_size, 0),
    mProcessedBuffer(buffer_size, 0)
{
}

float EnvelopeDetector::AttackFactor()
{
    return 0;
}

float EnvelopeDetector::DecayFactor()
{
    return 0;
}

float EnvelopeDetector::ProcessSample(float value)
{
    float retval = mProcessedBuffer[mPos];
    mLookaheadBuffer[mPos++] = value;
    if(mPos == mProcessingBuffer.size())
    {
        Follow();
        mPos = 0;
        mProcessedBuffer.swap(mProcessingBuffer);
        mLookaheadBuffer.swap(mProcessingBuffer);
    }
    return retval;
}

void EnvelopeDetector::CalcInitialCondition(float value)
{
}

size_t EnvelopeDetector::GetBlockSize() const
{
    assert(mProcessedBuffer.size() == mProcessingBuffer.size());
    assert(mProcessedBuffer.size() == mLookaheadBuffer.size());
    return mLookaheadBuffer.size();
}

const float* EnvelopeDetector::GetBuffer(int idx) const
{
    if(idx == 0)
        return mProcessedBuffer.data();
    else if(idx == 1)
        return mProcessingBuffer.data();
    else if(idx == 2)
        return mLookaheadBuffer.data();
    else
        assert(false);
    return nullptr;
}

//// ExpFitEnvelopeDetector implementation ////////////////////////////////////

ExpFitEnvelopeDetector::ExpFitEnvelopeDetector(
   float rate, float attackTime, float releaseTime, size_t bufferSize)
   : EnvelopeDetector(bufferSize)
{
    SetParams(rate, attackTime, releaseTime);
}

void ExpFitEnvelopeDetector::Reset(float value)
{
    std::fill(mProcessedBuffer.begin(), mProcessedBuffer.end(), value);
    std::fill(mProcessingBuffer.begin(), mProcessingBuffer.end(), value);
    std::fill(mLookaheadBuffer.begin(), mLookaheadBuffer.end(), value);
}

void ExpFitEnvelopeDetector::SetParams(
   float sampleRate, float attackTime, float releaseTime)
{
    attackTime = std::max(attackTime, 1.0f / sampleRate);
    releaseTime = std::max(releaseTime, 1.0f / sampleRate);
    mAttackFactor = exp(-1.0 / (sampleRate * attackTime));
    mReleaseFactor = exp(-1.0 / (sampleRate * releaseTime));
}

void ExpFitEnvelopeDetector::Follow()
{
    /*
    "Follow"ing algorithm by Roger B. Dannenberg, taken from
    Nyquist.  His description follows.  -DMM

    Description: this is a sophisticated envelope follower.
    The input is an envelope, e.g. something produced with
    the AVG function. The purpose of this function is to
    generate a smooth envelope that is generally not less
    than the input signal. In other words, we want to "ride"
    the peaks of the signal with a smooth function. The
    algorithm is as follows: keep a current output value
    (called the "value"). The value is allowed to increase
    by at most rise_factor and decrease by at most fall_factor.
    Therefore, the next value should be between
    value * rise_factor and value * fall_factor. If the input
    is in this range, then the next value is simply the input.
    If the input is less than value * fall_factor, then the
    next value is just value * fall_factor, which will be greater
    than the input signal. If the input is greater than value *
    rise_factor, then we compute a rising envelope that meets
    the input value by working backwards in time, changing the
    previous values to input / rise_factor, input / rise_factor^2,
    input / rise_factor^3, etc. until this NEW envelope intersects
    the previously computed values. There is only a limited buffer
    in which we can work backwards, so if the NEW envelope does not
    intersect the old one, then make yet another pass, this time
    from the oldest buffered value forward, increasing on each
    sample by rise_factor to produce a maximal envelope. This will
    still be less than the input.

    The value has a lower limit of floor to make sure value has a
    reasonable positive value from which to begin an attack.
    */
    assert(mProcessedBuffer.size() == mProcessingBuffer.size());
    assert(mProcessedBuffer.size() == mLookaheadBuffer.size());

    // First apply a peak detect with the requested release rate.
    size_t buffer_size = mProcessingBuffer.size();
    double env = mProcessedBuffer[buffer_size-1];
    for(size_t i = 0; i < buffer_size; ++i)
    {
        env *= mReleaseFactor;
        if(mProcessingBuffer[i] > env)
            env = mProcessingBuffer[i];
        mProcessingBuffer[i] = env;
    }
    // Preprocess lookahead buffer as well.
    for(size_t i = 0; i < buffer_size; ++i)
    {
        env *= mReleaseFactor;
        if(mLookaheadBuffer[i] > env)
            env = mLookaheadBuffer[i];
        mLookaheadBuffer[i] = env;
    }

    // Next do the same process in reverse direction to get the
    // requested attack rate and preprocess lookahead buffer.
    for(ssize_t i = buffer_size - 1; i >= 0; --i)
    {
        env *= mAttackFactor;
        if(mLookaheadBuffer[i] < env)
            mLookaheadBuffer[i] = env;
        else
            env = mLookaheadBuffer[i];
    }
    for(ssize_t i = buffer_size - 1; i >= 0; --i)
    {
        if(mProcessingBuffer[i] < env * mAttackFactor)
        {
            env *= mAttackFactor;
            mProcessingBuffer[i] = env;
        }
        else if(mProcessingBuffer[i] > env)
            // Intersected the previous envelope buffer, so we are finished
            return;
        else
            ; // Do nothing if we are on a plateau from peak look-around
    }
}


//// Pt1EnvelopeDetector implementation ///////////////////////////////////////
Pt1EnvelopeDetector::Pt1EnvelopeDetector(
    float rate, float attackTime, float releaseTime, size_t bufferSize,
    bool correctGain)
    : EnvelopeDetector(bufferSize),
    mCorrectGain(correctGain)
{
    SetParams(rate, attackTime, releaseTime);
}

float Pt1EnvelopeDetector::AttackFactor()
{
    return mAttackFactor;
}
float Pt1EnvelopeDetector::DecayFactor()
{
    return mReleaseFactor;
}

void Pt1EnvelopeDetector::Reset(float value)
{
    value *= mGainCorrection;
    std::fill(mProcessedBuffer.begin(), mProcessedBuffer.end(), value);
    std::fill(mProcessingBuffer.begin(), mProcessingBuffer.end(), value);
    std::fill(mLookaheadBuffer.begin(), mLookaheadBuffer.end(), value);
}

void Pt1EnvelopeDetector::SetParams(
    float sampleRate, float attackTime, float releaseTime)
{
    attackTime = std::max(attackTime, 1.0f / sampleRate);
    releaseTime = std::max(releaseTime, 1.0f / sampleRate);

    // Approximate peak amplitude correction factor.
    if(mCorrectGain)
        mGainCorrection = 1.0 + exp(attackTime / 30.0);
    else
        mGainCorrection = 1.0;

    mAttackFactor = 1.0 / (attackTime * sampleRate);
    mReleaseFactor  = 1.0 / (releaseTime  * sampleRate);
    mInitialBlockSize = std::min(size_t(sampleRate * sqrt(attackTime)), mLookaheadBuffer.size());
}

void Pt1EnvelopeDetector::CalcInitialCondition(float value)
{
    mLookaheadBuffer[mPos++] = value;
    if(mPos == mInitialBlockSize)
    {
        float level = 0;
        for(size_t i = 0; i < mPos; ++i)
        {
            if(mLookaheadBuffer[i] >= level)
                if(i < mInitialBlockSize / 5)
                level += 5 * mAttackFactor * (mLookaheadBuffer[i] - level);
                else
                level += mAttackFactor * (mLookaheadBuffer[i] - level);
            else
                level += mReleaseFactor * (mLookaheadBuffer[i] - level);
        }
        mInitialCondition = level;
        mPos = 0;
    }
}

void Pt1EnvelopeDetector::Follow()
{
    assert(mProcessedBuffer.size() == mProcessingBuffer.size());
    assert(mProcessedBuffer.size() == mLookaheadBuffer.size());

    // Simulate analog compressor with PT1 characteristic.
    size_t buffer_size = mProcessingBuffer.size();
    float level = mProcessedBuffer[buffer_size-1] / mGainCorrection;
    for(size_t i = 0; i < buffer_size; ++i)
    {
        if(mProcessingBuffer[i] >= level)
            level += mAttackFactor * (mProcessingBuffer[i] - level);
        else
            level += mReleaseFactor * (mProcessingBuffer[i] - level);
        mProcessingBuffer[i] = level * mGainCorrection;
    }
}

//// PipelineBuffer implementation ////////////////////////////////////////////
void PipelineBuffer::pad_to(size_t len, float value, bool stereo)
{
    if(size < len)
    {
        size = len;
        std::fill(mBlockBuffer[0].get() + trackSize,
            mBlockBuffer[0].get() + size, value);
        if(stereo)
            std::fill(mBlockBuffer[1].get() + trackSize,
                mBlockBuffer[1].get() + size, value);
    }
}

void PipelineBuffer::swap(PipelineBuffer& other)
{
    std::swap(trackPos, other.trackPos);
    std::swap(trackSize, other.trackSize);
    std::swap(size, other.size);
    std::swap(mBlockBuffer[0], other.mBlockBuffer[0]);
    std::swap(mBlockBuffer[1], other.mBlockBuffer[1]);
}

void PipelineBuffer::init(size_t capacity, bool stereo)
{
    trackPos = 0;
    trackSize = 0;
    size = 0;
    mCapacity = capacity;
    mBlockBuffer[0].reinit(capacity);
    if(stereo)
        mBlockBuffer[1].reinit(capacity);
    fill(0, stereo);
}

void PipelineBuffer::fill(float value, bool stereo)
{
    std::fill(mBlockBuffer[0].get(), mBlockBuffer[0].get() + mCapacity, value);
    if(stereo)
        std::fill(mBlockBuffer[1].get(), mBlockBuffer[1].get() + mCapacity, value);
}

void PipelineBuffer::free()
{
    mBlockBuffer[0].reset();
    mBlockBuffer[1].reset();
}
