/**********************************************************************

  Tenacity

  DynamicCompressorBase.cpp

  Max Maisel (based on Compressor effect)

**********************************************************************/

#include "DynamicCompressorBase.h"
#include "ComponentInterfaceSymbol.h"
#include "EffectInterface.h"
#include "EffectOutputTracks.h"
#include "ShuttleAutomation.h"

#include <cassert>

#if defined(DEBUG_COMPRESSOR2_DUMP_BUFFERS) or defined(DEBUG_COMPRESSOR2_TRACE2)
#include <fstream>
#include <string>
using namespace std::string_literals;
int buf_num;
std::fstream debugfile;
#endif

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

// DynamicCompressorInstance implementation

const EnumValueSymbol DynamicCompressorBase::kAlgorithmsStrings[nAlgos] =
{
   { XO("Exponential-Fit") },
   { XO("Analog Model") }
};

const ComponentInterfaceSymbol DynamicCompressorBase::kCompressByStrings[nBy] =
{
   { XO("peak amplitude") },
   { XO("RMS") }
};

const ComponentInterfaceSymbol DynamicCompressorBase::Symbol
{ XO("Dynamic Compressor") };

struct FactoryPreset
{
    const TranslatableString name;
    int algorithm;
    int compressBy;
    bool stereoInd;
    double thresholdDB;
    double ratio;
    double kneeWidthDB;
    double attackTime;
    double releaseTime;
    double lookaheadTime;
    double lookbehindTime;
    double outputGainDB;
};

static const FactoryPreset FactoryPresets[] =
{
    { XO("Dynamic Reduction"), kEnvPT1, kAmplitude, false, -40, 2.5, 6, 0.3, 0.3, 0.5, 0.5, 23 },
    { XO("Peak Reduction"), kEnvPT1, kAmplitude, false, -10, 10, 0, 0.001, 0.05, 0, 0, 0 },
    { XO("Analog Limiter"), kEnvPT1, kAmplitude, false, -6, 100, 6, 0.0001, 0.0001, 0, 0, 0 }
};

const EffectParameterMethods& DynamicCompressorBase::Parameters() const
{
    static CapturedParameters<
        DynamicCompressorBase,
        Algorithm,
        CompressBy,
        StereoInd,
        Threshold,
        Ratio,
        KneeWidth,
        AttackTime,
        ReleaseTime,
        LookaheadTime,
        LookbehindTime,
        OutputGain
    > parameters;
    return parameters;
}

DynamicCompressorBase::DynamicCompressorBase()
{
    Parameters().Reset(*this);

    mAlgorithm = Algorithm.def;
    mCompressBy = CompressBy.def;
    mStereoInd = StereoInd.def;

    mThresholdDB = Threshold.def;
    mRatio = Ratio.def;             // positive number > 1.0
    mKneeWidthDB = KneeWidth.def;
    mAttackTime = AttackTime.def;   // seconds
    mReleaseTime = ReleaseTime.def; // seconds
    mLookaheadTime = LookaheadTime.def;
    mLookbehindTime = LookbehindTime.def;
    mOutputGainDB = OutputGain.def;
}

// ComponentInterface implementation

ComponentInterfaceSymbol DynamicCompressorBase::GetSymbol() const
{
    return Symbol;
}

TranslatableString DynamicCompressorBase::GetDescription() const
{
    return XO("Reduces the dynamic of one or more tracks");
}

ManualPageID DynamicCompressorBase::ManualPage() const
{
    return L"Dynamic_Compressor";
}

// EffectDefinitionInterface implementation

EffectType DynamicCompressorBase::GetType() const
{
    return EffectTypeProcess;
}

EffectDefinitionInterface::RealtimeSince DynamicCompressorBase::RealtimeSupport() const
{
    return EffectDefinitionInterface::RealtimeSince::Never;
}

unsigned DynamicCompressorBase::GetAudioInCount() const
{
    return 2;
}

unsigned DynamicCompressorBase::GetAudioOutCount() const
{
    return 2;
}

bool DynamicCompressorBase::RealtimeInitialize(EffectSettings&, double sampleRate)
{
    SetBlockSize(512);
    AllocRealtimePipeline(sampleRate);
    UpdateRealtimeParams(sampleRate);
    return true;
}

bool DynamicCompressorBase::RealtimeAddProcessor(
   EffectSettings&, EffectOutputs*, unsigned /* numChannels */, float sampleRate)
{
    mProcStereo = true;
    mPreproc = InitPreprocessor(sampleRate);
    mEnvelope = InitEnvelope(sampleRate, mPipeline[0].size);

    mProgressVal = 0;

    #ifdef DEBUG_COMPRESSOR2_TRACE2
    debugfile.close();
    debugfile.open("/tmp/audio.out", std::ios::trunc | std::ios::out);
    #endif

return true;
}

bool DynamicCompressorBase::RealtimeFinalize(EffectSettings&) noexcept
{
    mPreproc.reset(nullptr);
    mEnvelope.reset(nullptr);
    FreePipeline();

    #ifdef DEBUG_COMPRESSOR2_TRACE2
        debugfile.close();
    #endif
    return true;
}

size_t DynamicCompressorBase::RealtimeProcess(size_t group, EffectSettings&, const float* const* inBuf,
    float* const* outBuf, size_t numSamples)
{
    std::lock_guard<std::mutex> guard(mRealtimeMutex);
    const size_t j = PIPELINE_DEPTH-1;
    float** outbuf = const_cast<float**>(outBuf);
    for(size_t i = 0; i < numSamples; ++i)
    {
        if(mPipeline[j].trackSize == mPipeline[j].size)
        {
            ProcessPipeline();
            mPipeline[j].trackSize = 0;
            SwapPipeline();
        }

        outbuf[0][i] = mPipeline[j][0][mPipeline[j].trackSize];
        outbuf[1][i] = mPipeline[j][1][mPipeline[j].trackSize];
        mPipeline[j][0][mPipeline[j].trackSize] = inBuf[0][i];
        mPipeline[j][1][mPipeline[j].trackSize] = inBuf[1][i];
        ++mPipeline[j].trackSize;
    }
    return numSamples;
}

// Effect implementation

bool DynamicCompressorBase::Process(
    EffectInstance &instance, EffectSettings &settings
)
{
    // Iterate over each track
    EffectOutputTracks outputTracks{*mTracks, GetType(), {{mT0, mT1}}};
    bool bGoodResult = true;

    AllocPipeline();
    double sampleRate;
    mProgressVal = 0;

    #ifdef DEBUG_COMPRESSOR2_TRACE2
    debugfile.close();
    debugfile.open("/tmp/audio.out", std::ios::trunc | std::ios::out);
    #endif

    for(const auto track : outputTracks.Get().Selected<WaveTrack>()
        + (mStereoInd ? &Track::Any : &Track::IsLeader))
    {
        // Get start and end times from track
        // PRL: No accounting for multiple channels ?
        double trackStart = track->GetStartTime();
        double trackEnd = track->GetEndTime();

        // Set the current bounds to whichever left marker is
        // greater and whichever right marker is less:
        mCurT0 = mT0 < trackStart? trackStart: mT0;
        mCurT1 = mT1 > trackEnd? trackEnd: mT1;

        // Get the track rate
        sampleRate = track->GetRate();

        auto range = mStereoInd
            ? TrackList::SingletonRange(track)
            : TrackList::Channels(track);

        mProcStereo = range.size() > 1;

        mPreproc = InitPreprocessor(sampleRate);
        mEnvelope = InitEnvelope(sampleRate, mPipeline[0].capacity());

        if(!ProcessOne(range))
        {
            // Processing failed -> abort
            bGoodResult = false;
            break;
        }
    }

    outputTracks.Commit();
    mPreproc.reset(nullptr);
    mEnvelope.reset(nullptr);
    FreePipeline();

    #ifdef DEBUG_COMPRESSOR2_TRACE2
    debugfile.close();
    #endif

    return bGoodResult;
}

RegistryPaths DynamicCompressorBase::GetFactoryPresets() const
{
    RegistryPaths names;

    for (size_t i = 0; i < sizeof(FactoryPresets) / sizeof(FactoryPreset); i++)
    {
        names.push_back(FactoryPresets[i].name.Translation());
    }

    return names;
}

OptionalMessage DynamicCompressorBase::LoadFactoryPreset(int id, EffectSettings&) const
{
    // TODO: externalize state so this cast isn't needed
    return const_cast<DynamicCompressorBase*>(this)->DoLoadFactoryPreset(id);
}

OptionalMessage DynamicCompressorBase::DoLoadFactoryPreset(int id)
{
    if (id < 0 || id >= int(sizeof(FactoryPresets) / sizeof(FactoryPreset)))
    {
        return {};
    }

    const FactoryPreset& preset = FactoryPresets[id];

    mAlgorithm = preset.algorithm;
    mCompressBy = preset.compressBy;
    mStereoInd = preset.stereoInd;

    mThresholdDB = preset.thresholdDB;
    mRatio = preset.ratio;
    mKneeWidthDB = preset.kneeWidthDB;
    mAttackTime = preset.attackTime;
    mReleaseTime = preset.releaseTime;
    mLookaheadTime = preset.lookaheadTime;
    mLookbehindTime = preset.lookbehindTime;
    mOutputGainDB = preset.outputGainDB;

    return { nullptr };
}

// DynamicCompressorBase implementation

double DynamicCompressorBase::CompressorGain(double env)
{
    double kneeCond;
    double envDB = LINEAR_TO_DB(env);

    // envDB can become NaN is env is exactly zero.
    // As solution, use a very low dB value to prevent NaN propagation.
    if (std::isnan(envDB))
        envDB = -200;

    kneeCond = 2.0 * (envDB - mThresholdDB);
    if(kneeCond < -mKneeWidthDB)
    {
        // Below threshold: only apply make-up gain
        return DB_TO_LINEAR(mOutputGainDB);
    }
    else if(kneeCond >= mKneeWidthDB)
    {
        // Above threshold: apply compression and make-up gain
        return DB_TO_LINEAR(mThresholdDB +
            (envDB - mThresholdDB) / mRatio + mOutputGainDB - envDB);
    }
    else
    {
        // Within knee: apply interpolated compression and make-up gain
        return DB_TO_LINEAR(
            (1.0 / mRatio - 1.0)
            * pow(envDB - mThresholdDB + mKneeWidthDB / 2.0, 2)
            / (2.0 * mKneeWidthDB) + mOutputGainDB);
    }
}

std::unique_ptr<SamplePreprocessor> DynamicCompressorBase::InitPreprocessor(
   double rate, bool preview)
{
    size_t window_size = CalcWindowLength(rate);
    if(mCompressBy == kAmplitude)
        return std::unique_ptr<SamplePreprocessor>(safenew
            SlidingMaxPreprocessor(window_size));
    else
        return std::unique_ptr<SamplePreprocessor>(safenew
            SlidingRmsPreprocessor(window_size, preview ? 1.0 : 2.0));
}

std::unique_ptr<EnvelopeDetector> DynamicCompressorBase::InitEnvelope(
   double rate, size_t blockSize, bool preview)
{
    if(mAlgorithm == kExpFit)
        return std::unique_ptr<EnvelopeDetector>(safenew
            ExpFitEnvelopeDetector(rate, mAttackTime, mReleaseTime, blockSize));
    else
        return std::unique_ptr<EnvelopeDetector>(safenew
            Pt1EnvelopeDetector(rate, mAttackTime, mReleaseTime, blockSize,
                !preview && mCompressBy != kAmplitude));
}

size_t DynamicCompressorBase::CalcBufferSize(double sampleRate)
{
    size_t capacity;
    mLookaheadLength = CalcLookaheadLength(sampleRate);
    capacity = mLookaheadLength +
        size_t(float(TAU_FACTOR) * (1.0 + mAttackTime) * sampleRate);
    if(capacity < MIN_BUFFER_CAPACITY)
        capacity = MIN_BUFFER_CAPACITY;
    return capacity;
}

size_t DynamicCompressorBase::CalcLookaheadLength(double rate)
{
    return std::max(0, int(round(mLookaheadTime * rate)));
}

size_t DynamicCompressorBase::CalcWindowLength(double rate)
{
    return std::max(1, int(round((mLookaheadTime + mLookbehindTime) * rate)));
}

/// Get required buffer size for the largest whole track and allocate buffers.
/// This reduces the amount of allocations required.
void DynamicCompressorBase::AllocPipeline()
{
    bool stereoTrackFound = false;
    double maxSampleRate = 0;
    size_t capacity;

    mProcStereo = false;

    EffectOutputTracks outputTracks{*mTracks, GetType(), {{mT0, mT1}}};
    for (const auto* track : outputTracks.Get().Selected<WaveTrack>() + &Track::Any)
    {
        maxSampleRate = std::max(maxSampleRate, track->GetRate());

        // There is a stereo track
        if(track->IsLeader())
            stereoTrackFound = true;
    }

    // Initiate a processing quad-buffer. This buffer will (most likely)
    // be shorter than the length of the track being processed.
    stereoTrackFound = stereoTrackFound && !mStereoInd;
    capacity = CalcBufferSize(maxSampleRate);
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        mPipeline[i].init(capacity, stereoTrackFound);
}

void DynamicCompressorBase::AllocRealtimePipeline(double sampleRate)
{
    mLookaheadLength = CalcLookaheadLength(sampleRate);
    size_t blockSize = std::max(mLookaheadLength, size_t(512));
    if(mAlgorithm == kExpFit)
    {
        size_t riseTime = round(5.0 * (0.1 + mAttackTime)) * sampleRate;
        blockSize = std::max(blockSize, riseTime);
    }
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
    {
        mPipeline[i].init(blockSize, true);
        mPipeline[i].size = blockSize;
    }
}

void DynamicCompressorBase::FreePipeline()
{
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        mPipeline[i].free();
}

void DynamicCompressorBase::SwapPipeline()
{
    #ifdef DEBUG_COMPRESSOR2_DUMP_BUFFERS
    std::string blockname = "/tmp/blockbuf."s + std::to_string(buf_num) + ".bin"s;
    std::cerr << "Writing to " << blockname << "\n" << std::flush;
    std::fstream blockbuffer = std::fstream();
    blockbuffer.open(blockname, std::ios::binary | std::ios::out);
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i) {
        float val = mPipeline[i].trackSize;
        blockbuffer.write((char*)&val, sizeof(float));
        val = mPipeline[i].size;
        blockbuffer.write((char*)&val, sizeof(float));
        val = mPipeline[i].capacity();
        blockbuffer.write((char*)&val, sizeof(float));
    }
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        blockbuffer.write((char*)mPipeline[i][0], mPipeline[i].capacity() * sizeof(float));

    std::string envname = "tmp/envbuf." + std::to_string(buf_num++) + ".bin"s;
    std::cerr << "Writing to " << envname << "\n" << std::flush;
    std::fstream envbuffer = std::fstream();
    envbuffer.open(envname, std::ios::binary | std::ios::out);
    envbuffer.write((char*)mEnvelope->GetBuffer(0),
        mEnvelope->GetBlockSize() * sizeof(float));
    envbuffer.write((char*)mEnvelope->GetBuffer(1),
        mEnvelope->GetBlockSize() * sizeof(float));
    envbuffer.write((char*)mEnvelope->GetBuffer(2),
        mEnvelope->GetBlockSize() * sizeof(float));

    std::cerr << "PipelineState: ";
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        std::cerr << !!mPipeline[i].size;
    std::cerr << " ";
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        std::cerr << !!mPipeline[i].trackSize;

    std::cerr << "\ntrackSize: ";
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        std::cerr << mPipeline[i].trackSize << " ";
    std::cerr << "\ntrackPos: ";
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        std::cerr << mPipeline[i].trackPos.as_size_t() << " ";
    std::cerr << "\nsize: ";
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
        std::cerr << mPipeline[i].size << " ";
    std::cerr << "\n" << std::flush;
    #endif

    for(size_t i = 0; i < PIPELINE_DEPTH-1; ++i)
        mPipeline[i].swap(mPipeline[i+1]);

    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "\n";
    #endif
}

/// ProcessOne() takes a track, transforms it to bunch of buffer-blocks,
/// and executes ProcessData, on it...
bool DynamicCompressorBase::ProcessOne(TrackIterRange<WaveTrack> range)
{
    WaveTrack* track = *range.begin();

    // Transform the marker timepoints to samples
    const auto start = track->TimeToLongSamples(mCurT0);
    const auto end   = track->TimeToLongSamples(mCurT1);

    // Get the length of the buffer (as double). len is
    // used simply to calculate a progress meter, so it is easier
    // to make it a double now than it is to do it later
    mTrackLen = (end - start).as_double();

    // Abort if the right marker is not to the right of the left marker
    if(mCurT1 <= mCurT0)
        return false;

    // Go through the track one buffer at a time. s counts which
    // sample the current buffer starts at.
    auto pos = start;

    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "ProcLen: " << (end - start).as_size_t() << "\n" << std::flush;
    std::cerr << "EnvBlockLen: " << mEnvelope->GetBlockSize() << "\n" << std::flush;
    std::cerr << "PipeBlockLen: " << mPipeline[0].capacity() << "\n" << std::flush;
    std::cerr << "LookaheadLen: " << mLookaheadLength << "\n" << std::flush;
    #endif

    bool first = true;
    mProgressVal = 0;

    #ifdef DEBUG_COMPRESSOR2_DUMP_BUFFERS
    buf_num = 0;
    #endif

    while(pos < end)
    {
        #ifdef DEBUG_COMPRESSOR2_TRACE
        std::cerr << "ProcessBlock at: " << pos.as_size_t() << "\n" << std::flush;
        #endif

        StorePipeline(range);
        SwapPipeline();

        const size_t remainingLen = (end - pos).as_size_t();

        // Get a block of samples (smaller than the size of the buffer)
        // Adjust the block size if it is the final block in the track
        const auto blockLen = limitSampleBufferSize(
            remainingLen, mPipeline[PIPELINE_DEPTH-1].capacity());

        mPipeline[PIPELINE_DEPTH-1].trackPos = pos;
        if(!LoadPipeline(range, blockLen))
            return false;

        if(first)
        {
            first = false;
            size_t sampleCount = mEnvelope->InitialConditionSize();
            for(size_t i = 0; i < sampleCount; ++i)
            {
                size_t rp = i % mPipeline[PIPELINE_DEPTH-1].trackSize;
                mEnvelope->CalcInitialCondition(
                PreprocSample(mPipeline[PIPELINE_DEPTH-1], rp));
            }
            mPipeline[PIPELINE_DEPTH-2].fill(
                mEnvelope->InitialCondition(), mProcStereo);
            mPreproc->Reset();
        }

        if(mPipeline[0].size == 0)
            FillPipeline();
        else
            ProcessPipeline();

        // Increment s one blockfull of samples
        pos += blockLen;

        if(!UpdateProgress())
            return false;
    }

    // Handle short selections
    while(mPipeline[1].size == 0)
    {
    #ifdef DEBUG_COMPRESSOR2_TRACE
        std::cerr << "PaddingLoop: ";
        for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
            std::cerr << !!mPipeline[i].size;
        std::cerr << " ";
        for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
            std::cerr << !!mPipeline[i].trackSize;
        std::cerr << "\n" << std::flush;
    #endif
        SwapPipeline();
        FillPipeline();
        if(!UpdateProgress())
            return false;
    }

    while(PipelineHasData())
    {
        StorePipeline(range);
        SwapPipeline();
        DrainPipeline();
        if(!UpdateProgress())
            return false;
    }
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "StoreLastBlock\n" << std::flush;
    #endif
    StorePipeline(range);

    // Return true because the effect processing succeeded ... unless cancelled
    return true;
}

bool DynamicCompressorBase::LoadPipeline(
   TrackIterRange<WaveTrack> range, size_t len)
{
    sampleCount read_size = -1;
    sampleCount last_read_size = -1;
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "LoadBlock at: " <<
        mPipeline[PIPELINE_DEPTH-1].trackPos.as_size_t() <<
        " with len: " << len << "\n" << std::flush;
    #endif
    // Get the samples from the track and put them in the buffer
    int idx = 0;
    for (auto track : range)
    {
        for (auto channel : track->Channels()) {
            channel->GetFloats(
                mPipeline[PIPELINE_DEPTH-1][idx],
                mPipeline[PIPELINE_DEPTH-1].trackPos, len,
                FillFormat::fillZero, true, &read_size
            );

            // WaveChannel::GetFloats returns the amount of read samples excluding zero
            // filled samples from clip gaps. But in case of stereo tracks with
            // asymetric gaps it still returns the same number for both channels.
            //
            // Fail if we read different sample count from stereo pair tracks.
            // Ignore this check during first iteration (last_read_size == -1).
            if(read_size != last_read_size && last_read_size.as_long_long() != -1)
                return false;
            mPipeline[PIPELINE_DEPTH-1].trackSize = read_size.as_size_t();
            mPipeline[PIPELINE_DEPTH-1].size = read_size.as_size_t();
            ++idx;
        }
    }

    assert(mPipeline[PIPELINE_DEPTH-2].trackSize == 0 ||
        mPipeline[PIPELINE_DEPTH-2].trackSize >=
        mPipeline[PIPELINE_DEPTH-1].trackSize);
    return true;
}

void DynamicCompressorBase::FillPipeline()
{
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "FillBlock: " <<
        !!mPipeline[0].size << !!mPipeline[1].size <<
        !!mPipeline[2].size << !!mPipeline[3].size <<
        "\n" << std::flush;
    std::cerr << "  from " << -int(mLookaheadLength)
        << " to " << mPipeline[PIPELINE_DEPTH-1].size - mLookaheadLength << "\n" << std::flush;
    std::cerr << "Padding from " << mPipeline[PIPELINE_DEPTH-1].trackSize
        << " to " << mEnvelope->GetBlockSize() << "\n" << std::flush;
    #endif
    // TODO: correct end conditions
    mPipeline[PIPELINE_DEPTH-1].pad_to(mEnvelope->GetBlockSize(), 0, mProcStereo);

    size_t length = mPipeline[PIPELINE_DEPTH-1].size;
    for(size_t rp = mLookaheadLength, wp = 0; wp < length; ++rp, ++wp)
    {
        if(rp < length)
            EnvelopeSample(mPipeline[PIPELINE_DEPTH-2], rp);
        else
            EnvelopeSample(mPipeline[PIPELINE_DEPTH-1], rp % length);
    }
}

void DynamicCompressorBase::ProcessPipeline()
{
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "ProcessBlock: " <<
        !!mPipeline[0].size << !!mPipeline[1].size <<
        !!mPipeline[2].size << !!mPipeline[3].size <<
        "\n" << std::flush;
    #endif
    float env;
    size_t length = mPipeline[0].size;

    for(size_t i = 0; i < PIPELINE_DEPTH-2; ++i)
        { assert(mPipeline[0].size == mPipeline[i+1].size); }

    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "LookaheadLen: " << mLookaheadLength << "\n" << std::flush;
    std::cerr << "PipeLength: " <<
        mPipeline[0].size << " " << mPipeline[1].size << " " <<
        mPipeline[2].size << " " << mPipeline[3].size <<
        "\n" << std::flush;
    #endif

    for(size_t rp = mLookaheadLength, wp = 0; wp < length; ++rp, ++wp)
    {
        if(rp < length)
            env = EnvelopeSample(mPipeline[PIPELINE_DEPTH-2], rp);
        else if((rp % length) < mPipeline[PIPELINE_DEPTH-1].size)
            env = EnvelopeSample(mPipeline[PIPELINE_DEPTH-1], rp % length);
        else
            // TODO: correct end condition
            env = mEnvelope->ProcessSample(mPreproc->ProcessSample(0.0));
        CompressSample(env, wp);
    }
}

inline float DynamicCompressorBase::PreprocSample(PipelineBuffer& pbuf, size_t rp)
{
    if(mProcStereo)
        return mPreproc->ProcessSample(pbuf[0][rp], pbuf[1][rp]);
    else
        return mPreproc->ProcessSample(pbuf[0][rp]);
}

inline float DynamicCompressorBase::EnvelopeSample(PipelineBuffer& pbuf, size_t rp)
{
    return mEnvelope->ProcessSample(PreprocSample(pbuf, rp));
}

inline void DynamicCompressorBase::CompressSample(float env, size_t wp)
{
    float gain = CompressorGain(env);

    #ifdef DEBUG_COMPRESSOR2_TRACE2
    float ThresholdDB = mThresholdDB;
    float Ratio = mRatio;
    float KneeWidthDB = mKneeWidthDB;
    float AttackTime = mAttackTime;
    float ReleaseTime = mReleaseTime;
    float LookaheadTime = mLookaheadTime;
    float LookbehindTime = mLookbehindTime;
    float OutputGainDB = mOutputGainDB;

    debugfile.write((char*)&ThresholdDB, sizeof(float));
    debugfile.write((char*)&Ratio, sizeof(float));
    debugfile.write((char*)&KneeWidthDB, sizeof(float));
    debugfile.write((char*)&AttackTime, sizeof(float));
    debugfile.write((char*)&ReleaseTime, sizeof(float));
    debugfile.write((char*)&LookaheadTime, sizeof(float));
    debugfile.write((char*)&LookbehindTime, sizeof(float));
    debugfile.write((char*)&OutputGainDB, sizeof(float));
    debugfile.write((char*)&mPipeline[0][0][wp], sizeof(float));
    if(mProcStereo)
        debugfile.write((char*)&mPipeline[0][1][wp], sizeof(float));
    debugfile.write((char*)&env, sizeof(float));
    debugfile.write((char*)&gain, sizeof(float));
    #endif

    #ifdef DEBUG_COMPRESSOR2_ENV
    if(wp < 100)
        mPipeline[0][0][wp] = 0;
    else
        mPipeline[0][0][wp] = env;
    #else
    mPipeline[0][0][wp] = mPipeline[0][0][wp] * gain;
    #endif
    if(mProcStereo)
        mPipeline[0][1][wp] = mPipeline[0][1][wp] * gain;

    #ifdef DEBUG_COMPRESSOR2_TRACE2
    debugfile.write((char*)&mPipeline[0][0][wp], sizeof(float));
    if(mProcStereo)
        debugfile.write((char*)&mPipeline[0][1][wp], sizeof(float));
    #endif
}

bool DynamicCompressorBase::PipelineHasData()
{
    for(size_t i = 0; i < PIPELINE_DEPTH; ++i)
    {
        if(mPipeline[i].size != 0)
            return true;
    }
    return false;
}

void DynamicCompressorBase::DrainPipeline()
{
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "DrainBlock: " <<
        !!mPipeline[0].size << !!mPipeline[1].size <<
        !!mPipeline[2].size << !!mPipeline[3].size <<
        "\n" << std::flush;
    bool once = false;
    #endif

    float env;
    size_t length = mPipeline[0].size;
    size_t length2 = mPipeline[PIPELINE_DEPTH-2].size;

    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "LookaheadLen: " << mLookaheadLength << "\n" << std::flush;
    std::cerr << "PipeLength: " <<
        mPipeline[0].size << " " << mPipeline[1].size << " " <<
        mPipeline[2].size << " " << mPipeline[3].size <<
        "\n" << std::flush;
    #endif

    for(size_t rp = mLookaheadLength, wp = 0; wp < length; ++rp, ++wp)
    {
        if(rp < length2 && mPipeline[PIPELINE_DEPTH-2].size != 0)
        {
    #ifdef DEBUG_COMPRESSOR2_TRACE
            if(!once)
            {
                once = true;
                std::cerr << "Draining overlapping buffer\n" << std::flush;
            }
    #endif
            env = EnvelopeSample(mPipeline[PIPELINE_DEPTH-2], rp);
        }
        else
            // TODO: correct end condition
            env = mEnvelope->ProcessSample(mPreproc->ProcessSample(0.0));
        CompressSample(env, wp);
    }
}

void DynamicCompressorBase::StorePipeline(TrackIterRange<WaveTrack> range)
{
    #ifdef DEBUG_COMPRESSOR2_TRACE
    std::cerr << "StoreBlock at: " << mPipeline[0].trackPos.as_size_t() <<
        " with len: " << mPipeline[0].trackSize << "\n" << std::flush;
    #endif

    int idx = 0;
    for (auto track : range)
    {
        for (auto channel : track->Channels()) {
            // Copy the newly-changed samples back onto the track.
            if (!channel->SetFloats(mPipeline[0][idx], mPipeline[0].trackPos, mPipeline[0].trackSize))
            {
                // TODO: Handle error
            } else
            {
                ++idx;
            }
        }
    }
    mPipeline[0].trackSize = 0;
    mPipeline[0].size = 0;
}

bool DynamicCompressorBase::UpdateProgress()
{
    mProgressVal +=
        (double(1+mProcStereo) * mPipeline[PIPELINE_DEPTH-1].trackSize)
        / (double(GetNumWaveTracks()) * mTrackLen);
    return !TotalProgress(mProgressVal);
}

void DynamicCompressorBase::UpdateRealtimeParams(double sampleRate)
{
    std::lock_guard<std::mutex> guard(mRealtimeMutex);
    size_t window_size = CalcWindowLength(sampleRate);
    mLookaheadLength = CalcLookaheadLength(sampleRate);
    mPreproc->SetWindowSize(window_size);
    mEnvelope->SetParams(sampleRate, mAttackTime, mReleaseTime);
}

