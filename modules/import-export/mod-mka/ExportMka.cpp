// SPDX-License-Identifier: GPL-2.0-or-later
/**********************************************************************

  Tenacity: A Digital Audio Editor

  ExportMka.cpp

  Steve Lhomme

**********************************************************************/

#include "CodeConversions.h"
#include "ExportOptionsEditor.h"
#include "ExportPlugin.h"
#include "ExportPluginHelpers.h"
#include "ExportPluginRegistry.h"
#include "ExportTypes.h"
#include "LabelTrack.h"
#include "Mix.h"
#include "PlainExportOptionsEditor.h"
#include "Project.h"
#include "SampleFormat.h"
#include "Tags.h"
#include "Track.h"
#include "WaveTrack.h"

#include <cstddef>
#include <memory>

#include <rapidjson/document.h>

#if defined(_CRTDBG_MAP_ALLOC) && LIBMATROSKA_VERSION < 0x010702
// older libmatroska headers use std::nothrow which is incompatible with <crtdbg.h>
#undef new
#endif

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlVoid.h>
#include <ebml/StdIOCallback.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxVersion.h>

#if LIBMATROSKA_VERSION < 0x010700
typedef enum {
  MATROSKA_TRACK_TYPE_VIDEO            = 0x1, // An image.
  MATROSKA_TRACK_TYPE_AUDIO            = 0x2, // Audio samples.
  MATROSKA_TRACK_TYPE_COMPLEX          = 0x3, // A mix of different other TrackType. The codec needs to define how the `Matroska Player` should interpret such data.
  MATROSKA_TRACK_TYPE_LOGO             = 0x10, // An image to be rendered over the video track(s).
  MATROSKA_TRACK_TYPE_SUBTITLE         = 0x11, // Subtitle or closed caption data to be rendered over the video track(s).
  MATROSKA_TRACK_TYPE_BUTTONS          = 0x12, // Interactive button(s) to be rendered over the video track(s).
  MATROSKA_TRACK_TYPE_CONTROL          = 0x20, // Metadata used to control the player of the `Matroska Player`.
  MATROSKA_TRACK_TYPE_METADATA         = 0x21, // Timed metadata that can be passed on to the `Matroska Player`.
} MatroskaTrackType;

typedef enum {
  MATROSKA_TARGET_TYPE_COLLECTION       = 70, // The highest hierarchical level that tags can describe.
  MATROSKA_TARGET_TYPE_EDITION          = 60, // A list of lower levels grouped together.
  MATROSKA_TARGET_TYPE_ALBUM            = 50, // The most common grouping level of music and video (equals to an episode for TV series).
  MATROSKA_TARGET_TYPE_PART             = 40, // When an album or episode has different logical parts.
  MATROSKA_TARGET_TYPE_TRACK            = 30, // The common parts of an album or movie.
  MATROSKA_TARGET_TYPE_SUBTRACK         = 20, // Corresponds to parts of a track for audio (like a movement).
  MATROSKA_TARGET_TYPE_SHOT             = 10, // The lowest hierarchy found in music or movies.
} MatroskaTargetTypeValue;
#endif

#ifdef USE_LIBFLAC
#include "FLAC++/encoder.h"
#endif

using namespace libmatroska;

//// Options ///////////////////////////////////////////////////////////////////

enum MkaOptionIDs
{
    MkaOptionFormatID,
    MkaOptionKeepLabelsID
};

const std::initializer_list<PlainExportOptionsEditor::OptionDesc> MkaOptions {
    {
        {
            MkaOptionFormatID, XO("Format"),
            "16", ExportOption::TypeEnum,
            {
                "16",
                "24",
                "f32",
                #ifdef USE_LIBFLAC
                "flac16",
                "flac24"
                #endif
            },
            {
                XO("PCM 16-bit (Little Endian)") ,
                XO("PCM 24-bit (Little Endian)") ,
                XO("PCM Float 32-bit") ,
                #ifdef USE_LIBFLAC
                XO("FLAC 16-bit"),
                XO("FLAC 24-bit")
                #endif
            }
        }, wxT("/FileFormats/MKA/Format")
    },
    {
        {
            MkaOptionKeepLabelsID, XO("Keep Labels"),
            true
        }, wxT("/FileFormats/MKA/ExportLabels")
    }
};

//// Utilities for the exporter ///////////////////////////////////////////////
static uint64_t GetRandomUID64()
{
    uint64_t uid = 0;
    for (size_t i=0; i<(64-7); i+=7)
    {
        auto r = static_cast<uint64_t>(std::rand() & 0x7fff);
        uid = uid << 7 | r;
    }
    return uid;
}

static void FillRandomUUID(binary UID[16])
{
    // This was originally in ExportMka::ExportMka, but I've migrated it here.
    std::srand(std::time(nullptr));

    uint64_t rand;
    rand = GetRandomUID64();
    memcpy(&UID[0], &rand, 8);
    rand = GetRandomUID64();
    memcpy(&UID[8], &rand, 8);
}

static void SetMetadata(const Tags *tags, KaxTag * & PrevTag, KaxTags & Tags, const wxChar *tagName, const MatroskaTargetTypeValue TypeValue, const wchar_t *mkaName)
{
    if (tags != nullptr && tags->HasTag(tagName))
    {
        KaxTag &tag = AddNewChild<KaxTag>(Tags);
        KaxTagTargets &tagTarget = GetChild<KaxTagTargets>(tag);
        (EbmlUInteger &) GetChild<KaxTagTargetTypeValue>(tagTarget) = TypeValue;

        KaxTagSimple &simpleTag = GetChild<KaxTagSimple>(tag);
        (EbmlUnicodeString &) GetChild<KaxTagName>(simpleTag) = (UTFstring)mkaName;
        (EbmlUnicodeString &) GetChild<KaxTagString>(simpleTag) = (UTFstring)tags->GetTag(tagName);
    }
}

constexpr uint64 MS_PER_FRAME = 40;

class ClusterMuxer
{
public:
    struct MuxerTime
    {
        uint64        prevEndTime{0};
        uint64        samplesRead{0};

        /*const*/ uint64  TimeUnit;
        /*const*/ double  rate;

        // MuxerTime(uint64 _TimeUnit, double _rate)
        //     :TimeUnit(_TimeUnit)
        //     ,rate(_rate)
        // {
        // }

        // MuxerTime(const MuxerTime & source) = default;

        void AddSample(size_t samplesThisRun)
        {
            samplesRead += samplesThisRun;
            // in rounded TimeUnit as the drift with the actual time accumulates
            prevEndTime = std::llround(samplesRead * 1000000000. / (TimeUnit * rate));
        }
    };

public:
    ClusterMuxer(KaxSegment &_FileSegment, KaxTrackEntry &_Track, KaxCues &_AllCues, MuxerTime _muxerTime)
        :FileSegment(_FileSegment)
        ,MyTrack1(_Track)
        ,AllCues(_AllCues)
        ,Time(_muxerTime)
        ,maxFrameSamples(MS_PER_FRAME * _muxerTime.rate / 1000) // match mkvmerge
    {

        Cluster = std::make_unique<KaxCluster>();
        Cluster->SetParent(FileSegment); // mandatory to store references in this Cluster
        Cluster->InitTimecode(Time.prevEndTime, Time.TimeUnit);
        Cluster->EnableChecksum();

        framesBlob = std::make_unique<KaxBlockBlob>(BLOCK_BLOB_SIMPLE_AUTO);
        framesBlob->SetParent(*Cluster);
        AllCues.AddBlockBlob(*framesBlob);
    }

    // return true when the Muxer is full
    bool AddBuffer(DataBuffer &dataBuff, size_t samplesThisRun)
    {
        if (!framesBlob)
        {
            framesBlob = std::make_unique<KaxBlockBlob>(BLOCK_BLOB_SIMPLE_AUTO);
            framesBlob->SetParent(*Cluster);
        }
        // TODO check the timestamp is OK within the Cluster first
        if (!framesBlob->AddFrameAuto(MyTrack1, Time.prevEndTime * Time.TimeUnit, dataBuff, LACING_NONE))
        {
            // last frame allowed in the lace, we need a new frame blob
            FinishFrameBlock(maxFrameSamples - samplesThisRun);
        }

        Time.AddSample(samplesThisRun);

        sampleWritten += samplesThisRun;

        if (sampleWritten >= (uint64)(10 * maxFrameSamples))
            return true;
        return false;
    }

    MuxerTime Finish(IOCallback & mka_file, KaxSeekHead & MetaSeek)
    {
        if (sampleWritten)
        {
            FinishFrameBlock(maxFrameSamples);
            Cluster->Render(mka_file, AllCues);
            // No need to use SeekHead for Cluster, there's Cues for that
            // MetaSeek.IndexThis(*Cluster, FileSegment);
        }
        return Time;
    }

private:
    void FinishFrameBlock(const size_t lessSamples)
    {
        if (framesBlob)
        {
            if (lessSamples != 0)
            {
                // last block, write the duration
                // TODO get frames from the blob and add them in a BLOCK_BLOB_NO_SIMPLE one
                // framesBlob->SetBlockDuration(lessSamples * Time.TimestampScale / Time.rate);
            }
            Cluster->AddBlockBlob(framesBlob.release());
        }
    }

    KaxSegment    &FileSegment;
    KaxTrackEntry &MyTrack1;
    KaxCues       &AllCues;
    const size_t  maxFrameSamples;

    MuxerTime                     Time;

    std::unique_ptr<KaxCluster>   Cluster;
    std::unique_ptr<KaxBlockBlob> framesBlob;
    uint64_t                      sampleWritten{0};
};

#ifdef USE_LIBFLAC
class MkaFLACEncoder : public FLAC::Encoder::Stream
{
public:
    ::FLAC__StreamEncoderInitStatus init() override
    {
        auto ret = FLAC::Encoder::Stream::init();
        initializing = false;
        return ret;
    }

    const std::vector<FLAC__byte> & GetInitBuffer()
    {
        return initBuffer;
    }

    bool Process(std::unique_ptr<ClusterMuxer> & Muxer, const FLAC__int32 * const buffer[], uint32_t samples)
    {
        muxer     = &Muxer;
        muxer_full = false;
        bool ret = process(buffer, samples);
        if (!ret)
            return true; // on error assume the Cluster is full
        return muxer_full;
    }

    bool finish() override
    {
        finishing = true;
        return FLAC::Encoder::Stream::finish();
    }

    bool hasNewCodecPrivate() const
    {
        return initializing || overwriting;
    }

protected:
    ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t /*current_frame*/) override
    {
        if (initializing)
        {
            std::size_t size = initBuffer.size();
            initBuffer.resize(size + bytes);
            memcpy(&initBuffer[size], buffer, bytes);
            return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
        }
        if (overwriting)
        {
            if (initOffset + bytes > initBuffer.size())
            {
                // we can't expand the CodePrivate
                return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
            }
            memcpy(&initBuffer[initOffset], buffer, bytes);
            return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
        }

        DataBuffer *dataBuff = new DataBuffer((binary*)buffer, bytes, nullptr, true);
        if ((*muxer)->AddBuffer(*dataBuff, samples))
            muxer_full = true;

        return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

    ::FLAC__StreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) override
    {
        // support overwriting the CodecPrivate with the checksum
        if (finishing && absolute_byte_offset < (initBuffer.size() + initOffset))
        {
            overwriting = true;
            initOffset = absolute_byte_offset;
            return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
        }
        return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
    }

    bool initializing{true};
    bool finishing{false};
    bool overwriting{false};
    std::vector<FLAC__byte> initBuffer;
    FLAC__uint64 initOffset{0};

    std::unique_ptr<ClusterMuxer> *muxer = nullptr;
    bool                          muxer_full;
};
#endif
///////////////////////////////////////////////////////////////////////////////

//// Export Processor /////////////////////////////////////////////////////////

/// Responsible for handling exports of Matroska (MKA/MKV) files.
class MkaExportProcessor final : public ExportProcessor
{
    private:
        struct
        {
            std::string                      bitDepthPref;
            bool                             keepLabels;
            double                           sampleRate;
            double                           t0, t1;
            unsigned                         numChannels;
            sampleFormat                     format;
            uint64_t                         bytesPerSample;
            std::string                      codecID;
            bool                             outInterleaved;
            std::unique_ptr<StdIOCallback>   mkaFile;
            std::unique_ptr<Mixer>           mixer;
            const Tags*                      metadata;
            TranslatableString               statusString;
            std::shared_ptr<AudacityProject> project; // FIXME: Find a way to get rid of this field

            // These should be constants
            size_t samplesPerRun;
            uint64 timestampUnit;

            EbmlVoid      dummyStart;
            KaxSegment    fileSegment;
            KaxCues       allCues;
            KaxSeekHead   metaSeek;
            KaxTrackEntry track1;

            #ifdef USE_LIBFLAC
            std::unique_ptr<MkaFLACEncoder> flacEncoder;
            #endif
        } context;

    public:
        bool Initialize(AudacityProject& project,
        const Parameters& parameters,
        const wxFileNameWrapper& filename,
        double t0, double t1, bool selectedOnly,
        double rate, unsigned channels,
        MixerOptions::Downmix* mixerSpec = nullptr,
        const Tags* metadata = nullptr) override;
   
        ExportResult Process(ExportProcessorDelegate& delegate) override;
};

bool MkaExportProcessor::Initialize(
    AudacityProject& project, const Parameters& parameters,
    const wxFileNameWrapper& filename,
    double t0, double t1, bool selectedOnly, double rate,
    unsigned channels, MixerOptions::Downmix* mixerSpec,
    const Tags* metadata
)
{
    //// First, gather our preferences and important info.
    //// To see where these defaults were taken, please consult MkaOptions.
    context.bitDepthPref = ExportPluginHelpers::GetParameterValue<std::string>(
        parameters, MkaOptionFormatID, "16"
    );

    context.keepLabels = ExportPluginHelpers::GetParameterValue<bool>(
        parameters, MkaOptionKeepLabelsID, true
    );

    context.sampleRate = rate;
    context.t0 = t0;
    context.t1 = t1;
    context.numChannels = channels;

    if (context.bitDepthPref == "24")
    {
        context.codecID = "A_PCM/INT/LIT";
        context.format = int24Sample;
        context.bytesPerSample = 3 * channels;
        context.outInterleaved = true;
    }
    else if (context.bitDepthPref == "16")
    {
        context.codecID = "A_PCM/INT/LIT";
        context.format = int16Sample;
        context.bytesPerSample = 2 * channels;
        context.outInterleaved = true;
    }
    else if (context.bitDepthPref == "f32")
    {
        context.codecID = "A_PCM/FLOAT/IEEE";
        context.format = floatSample;
        context.bytesPerSample = 4 * channels;
        context.outInterleaved = true;
    }
    #ifdef USE_LIBFLAC
    else if (context.bitDepthPref == "flac16")
    {
        context.codecID = "A_FLAC";
        context.format = int16Sample;
        context.bytesPerSample = 2 * channels;
        context.outInterleaved = false;
    }
    else if (context.bitDepthPref == "flac24")
    {
        context.codecID = "A_FLAC";
        context.format = int24Sample;
        context.bytesPerSample = 3 * channels;
        context.outInterleaved = false;
    }
    #endif

    //// Next, setup the file for export.
    auto fName = filename;
    fName.MakeAbsolute();
    auto path = fName.GetFullPath();
    context.mkaFile.reset(new StdIOCallback(path, MODE_CREATE));

    //// Then, setup and write the file structure, including any metadata.
    EbmlHead FileHead;
    (EbmlString &) GetChild<EDocType>(FileHead) = "matroska";

    if constexpr (LIBMATROSKA_VERSION >= 0x010406)
    {
        (EbmlUInteger &) GetChild<EDocTypeVersion>(FileHead) = 4; // needed for LanguageBCP47
    } else
    {
        (EbmlUInteger &) GetChild<EDocTypeVersion>(FileHead) = 2;
    }

    (EbmlUInteger &) GetChild<EDocTypeReadVersion>(FileHead) = 2; // needed for SimpleBlock
    (EbmlUInteger &) GetChild<EMaxIdLength>(FileHead) = 4;
    (EbmlUInteger &) GetChild<EMaxSizeLength>(FileHead) = 8;
    FileHead.Render(*context.mkaFile, true);

    auto SegmentSize = context.fileSegment.WriteHead(*context.mkaFile, 5);

    // reserve some space for the Meta Seek writen at the end
    context.dummyStart.SetSize(128);
    context.dummyStart.Render(*context.mkaFile);

    context.metaSeek.EnableChecksum();

    // Write program information (and related) to the file
    context.timestampUnit = std::llround(UINT64_C(1000000000) / rate);

    EbmlMaster & MyInfos = GetChild<KaxInfo>(context.fileSegment);
    MyInfos.EnableChecksum();
    (EbmlFloat &) GetChild<KaxDuration>(MyInfos) = (t1 - t0) * UINT64_C(1000000000) / context.timestampUnit; // in TIMESTAMP_UNIT
    GetChild<KaxDuration>(MyInfos).SetPrecision(EbmlFloat::FLOAT_64);
    (EbmlUnicodeString &) GetChild<KaxMuxingApp>(MyInfos)  = audacity::ToWString(std::string("libebml ") + EbmlCodeVersion + std::string(" + libmatroska ") + KaxCodeVersion);
    (EbmlUnicodeString &) GetChild<KaxWritingApp>(MyInfos) = audacity::ToWString(APP_NAME) + L" " + TENACITY_VERSION_STRING;
    (EbmlUInteger &) GetChild<KaxTimecodeScale>(MyInfos) = context.timestampUnit;
    GetChild<KaxDateUTC>(MyInfos).SetEpochDate(time(nullptr));
    binary SegUID[16];
    FillRandomUUID(SegUID);
    GetChild<KaxSegmentUID>(MyInfos).CopyBuffer(SegUID, 16);
    filepos_t InfoSize = MyInfos.Render(*context.mkaFile);
    if (InfoSize != 0)
    {
        context.metaSeek.IndexThis(MyInfos, context.fileSegment);
    }

    // Write track name
    // TODO: Multiple tracks
    auto& tracks = TrackList::Get(project);

    KaxTracks & MyTracks = GetChild<KaxTracks>(context.fileSegment);
    MyTracks.EnableChecksum();

    KaxTrackEntry & MyTrack1 = GetChild<KaxTrackEntry>(MyTracks);
    MyTrack1.SetGlobalTimecodeScale(context.timestampUnit);

    (EbmlUInteger &) GetChild<KaxTrackType>(MyTrack1) = MATROSKA_TRACK_TYPE_AUDIO;
    (EbmlUInteger &) GetChild<KaxTrackNumber>(MyTrack1) = 1;
    (EbmlUInteger &) GetChild<KaxTrackUID>(MyTrack1) = GetRandomUID64();
    (EbmlUInteger &) GetChild<KaxTrackDefaultDuration>(MyTrack1) = MS_PER_FRAME * 1000000;
    (EbmlString &) GetChild<KaxTrackLanguage>(MyTrack1) = "und";
    if constexpr (LIBMATROSKA_VERSION >= 0x010406)
    {
        (EbmlString &) GetChild<KaxLanguageIETF>(MyTrack1) = "und";
    }
    auto waveTracks = tracks.Selected< const WaveTrack >();
    auto pT = waveTracks.begin();
    if (*pT)
    {
        const auto sTrackName = (*pT)->GetName();
        if (!sTrackName.empty() && sTrackName != (*pT)->GetDefaultAudioTrackNamePreference())
        {
            (EbmlUnicodeString &) GetChild<KaxTrackName>(MyTrack1) = (UTFstring)sTrackName;
        }
    }

    // Write track info
    EbmlMaster & MyTrack1Audio = GetChild<KaxTrackAudio>(MyTrack1);
    (EbmlFloat &) GetChild<KaxAudioSamplingFreq>(MyTrack1Audio) = rate;
    (EbmlUInteger &) GetChild<KaxAudioChannels>(MyTrack1Audio) = channels;
    (EbmlString &) GetChild<KaxCodecID>(MyTrack1) = context.codecID;
    switch(context.format)
    {
        case int16Sample:
            (EbmlUInteger &) GetChild<KaxAudioBitDepth>(MyTrack1Audio) = 16;
            break;
        case int24Sample:
            (EbmlUInteger &) GetChild<KaxAudioBitDepth>(MyTrack1Audio) = 24;
            break;
        case floatSample:
            (EbmlUInteger &) GetChild<KaxAudioBitDepth>(MyTrack1Audio) = 32;
            break;
        default:
            return false;
    }

    // If FLAC support is enabled, and the user wants a FLAC export, setup the FLAC encoder
    #ifdef USE_LIBFLAC
    if (context.bitDepthPref == "flac16" || context.bitDepthPref == "flac24")
    {
        context.flacEncoder.reset(new MkaFLACEncoder);

        context.flacEncoder->set_bits_per_sample(context.format == int24Sample ? 24 : 16);

        context.flacEncoder->set_channels(channels) &&
        context.flacEncoder->set_sample_rate(lrint(rate));
        auto status = context.flacEncoder->init();
        if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
        {
            throw new std::runtime_error("toto");
        }
        const auto & buf = context.flacEncoder->GetInitBuffer();
        GetChild<KaxCodecPrivate>(MyTrack1).CopyBuffer(buf.data(), buf.size());
    }
    #endif

    filepos_t TrackSize = MyTracks.Render(*context.mkaFile);
    if (TrackSize != 0)
    {
        context.metaSeek.IndexThis(MyTracks, context.fileSegment);
    }

    // reserve some space after the track (to match mkvmerge for now)
    // EbmlVoid DummyTrack;
    // DummyTrack.SetSize(1068);
    // DummyTrack.Render(mka_file);

    // Add tags
    KaxTags & Tags = GetChild<KaxTags>(context.fileSegment);
    Tags.EnableChecksum();
    if (metadata == nullptr)
    {
        metadata = &Tags::Get(project);
    }
    KaxTag *prevTag = nullptr;
    SetMetadata(metadata, prevTag, Tags, TAG_TITLE,     MATROSKA_TARGET_TYPE_TRACK, L"TITLE");
    SetMetadata(metadata, prevTag, Tags, TAG_GENRE,     MATROSKA_TARGET_TYPE_TRACK, L"GENRE");
    SetMetadata(metadata, prevTag, Tags, TAG_ARTIST,    MATROSKA_TARGET_TYPE_ALBUM, L"ARTIST");
    SetMetadata(metadata, prevTag, Tags, TAG_ALBUM,     MATROSKA_TARGET_TYPE_ALBUM, L"TITLE");
    SetMetadata(metadata, prevTag, Tags, TAG_TRACK,     MATROSKA_TARGET_TYPE_ALBUM, L"PART_NUMBER");
    SetMetadata(metadata, prevTag, Tags, TAG_YEAR,      MATROSKA_TARGET_TYPE_ALBUM, L"DATE_RELEASED");
    SetMetadata(metadata, prevTag, Tags, TAG_COMMENTS,  MATROSKA_TARGET_TYPE_ALBUM, L"COMMENT");
    SetMetadata(metadata, prevTag, Tags, TAG_COPYRIGHT, MATROSKA_TARGET_TYPE_ALBUM, L"COPYRIGHT");
    filepos_t TagsSize = Tags.Render(*context.mkaFile);
    if (TagsSize != 0)
    {
        context.metaSeek.IndexThis(Tags, context.fileSegment);
    }

    context.allCues.SetGlobalTimecodeScale(context.timestampUnit);
    context.allCues.EnableChecksum();

    // If the user wants labels exported as chapters, collect the names
    context.project = std::move(project.shared_from_this());

    //// Finally, setup the mixer
    const size_t maxFrameSamples = MS_PER_FRAME * rate / 1000; // match mkvmerge
    context.samplesPerRun = maxFrameSamples * context.bytesPerSample;
    context.mixer = ExportPluginHelpers::CreateMixer(
        project, selectedOnly, t0, t1, channels, context.samplesPerRun,
        context.outInterleaved, rate, context.format, mixerSpec
    );

    //// Miscellaneous: take care of the status string
    context.statusString = selectedOnly ?
        XO("Exporting the selected audio as MKA") :
        XO("Exporting the audio as MKA");

    return true;
}

ExportResult MkaExportProcessor::Process(ExportProcessorDelegate& delegate)
{
    delegate.SetStatusString(context.statusString);
    ExportResult exportResult = ExportResult::Success;

    // References needed that we can't keep in our 'context' object.
    KaxTracks & MyTracks = GetChild<KaxTracks>(context.fileSegment);
    KaxTrackEntry & MyTrack1 = GetChild<KaxTrackEntry>(MyTracks);

    try
    {
        // add clusters
        std::unique_ptr<ClusterMuxer> Muxer;
        #ifdef USE_LIBFLAC
        ArraysOf<FLAC__int32> splitBuff{ context.numChannels, context.samplesPerRun, true };
        #endif

        ClusterMuxer::MuxerTime prevTime{0, 0, context.timestampUnit, context.sampleRate};
        while (exportResult == ExportResult::Success)
        {
            auto samplesThisRun = context.mixer->Process(context.samplesPerRun);
            if (samplesThisRun == 0)
            {
                #ifdef USE_LIBFLAC
                if (context.flacEncoder)
                {
                    if (!Muxer)
                    {
                        Muxer = std::make_unique<ClusterMuxer>(context.fileSegment, MyTrack1, context.allCues, prevTime);
                    }
                    context.flacEncoder->finish();
                }
                #endif

                if (Muxer)
                {
                    prevTime = Muxer->Finish(*context.mkaFile, context.metaSeek);
                    Muxer = nullptr;
                }
                break; //finished
            }

            if (!Muxer)
            {
                Muxer = std::make_unique<ClusterMuxer>(context.fileSegment, MyTrack1, context.allCues, prevTime);
            }

            #ifdef USE_LIBFLAC
            if (context.flacEncoder)
            {
                for (size_t i = 0; i < context.numChannels; i++)
                {
                    auto mixed = context.mixer->GetBuffer(i);
                    if (context.format == int24Sample) {
                        for (decltype(samplesThisRun) j = 0; j < samplesThisRun; j++)
                            splitBuff[i][j] = ((const int *)mixed)[j];
                    }
                    else {
                        for (decltype(samplesThisRun) j = 0; j < samplesThisRun; j++)
                            splitBuff[i][j] = ((const short *)mixed)[j];
                    }
                }
                auto b = reinterpret_cast<FLAC__int32**>( splitBuff.get() );
                if (context.flacEncoder->Process(Muxer, b, samplesThisRun))
                {
                    prevTime = Muxer->Finish(*context.mkaFile, context.metaSeek);
                    Muxer = nullptr;
                }
            }
            else
            #endif
            {
                auto mixed = context.mixer->GetBuffer();
                DataBuffer *dataBuff = new DataBuffer((binary*)mixed, samplesThisRun * context.bytesPerSample, nullptr, true);
                if (Muxer->AddBuffer(*dataBuff, samplesThisRun))
                {
                    prevTime = Muxer->Finish(*context.mkaFile, context.metaSeek);
                    Muxer = nullptr;
                }
            }

            exportResult = ExportPluginHelpers::UpdateProgress(delegate, *context.mixer, context.t0, context.t1);
        }

        // add cues
        filepos_t CueSize = context.allCues.Render(*context.mkaFile);
        if (CueSize != 0)
        {
            context.metaSeek.IndexThis(context.allCues, context.fileSegment);
        }

        uint64 lastElementEnd = context.allCues.GetEndPosition();

        // Add label tracks as chapters
        if (context.keepLabels)
        {
            const auto trackRange = TrackList::Get(*context.project).Any<const LabelTrack>();
            if (!trackRange.empty())
            {
                KaxChapters & EditionList = GetChild<KaxChapters>(context.fileSegment);
                for (const auto *lt : trackRange)
                {
                    if (lt->GetNumLabels())
                    {
                        // Create an edition with the track name
                        KaxEditionEntry &Edition = AddNewChild<KaxEditionEntry>(EditionList);
                        (EbmlUInteger &) GetChild<KaxEditionUID>(Edition) = GetRandomUID64();
                        if (!lt->GetName().empty() && lt->GetName() != lt->GetDefaultName())
                        {
#if LIBMATROSKA_VERSION >= 0x010700
                            KaxEditionDisplay & EditionDisplay = GetChild<KaxEditionDisplay>(Edition);
                            (EbmlUnicodeString &) GetChild<KaxEditionString>(EditionDisplay) = (UTFstring)lt->GetName();
#endif
                            // TODO also write the Edition name in tags for older Matroska parsers
                        }

                        // Add markers and selections
                        for (const auto & label : lt->GetLabels())
                        {
                            KaxChapterAtom & Chapter = AddNewChild<KaxChapterAtom>(Edition);
                            (EbmlUInteger &) GetChild<KaxChapterUID>(Chapter) = GetRandomUID64();
                            (EbmlUInteger &) GetChild<KaxChapterTimeStart>(Chapter) = label.getT0() * UINT64_C(1000000000);
                            if (label.getDuration() != 0.0)
                                (EbmlUInteger &) GetChild<KaxChapterTimeEnd>(Chapter) = label.getT1() * UINT64_C(1000000000);
                            if (!label.title.empty())
                            {
                                KaxChapterDisplay & ChapterDisplay = GetChild<KaxChapterDisplay>(Chapter);
                                (EbmlUnicodeString &) GetChild<KaxChapterString>(ChapterDisplay) = (UTFstring)label.title;
                                (EbmlString &) GetChild<KaxChapterLanguage>(ChapterDisplay) = "und";
#if LIBMATROSKA_VERSION >= 0x010600
                                (EbmlString &) GetChild<KaxChapLanguageIETF>(ChapterDisplay) = "und";
#endif
                            }
                        }
                    }
                }
                filepos_t ChaptersSize = EditionList.Render(*context.mkaFile);
                if (ChaptersSize != 0)
                {
                    context.metaSeek.IndexThis(EditionList, context.fileSegment);
                    lastElementEnd = EditionList.GetEndPosition();
                }
            }
        }

        auto MetaSeekSize = context.dummyStart.ReplaceWith(context.metaSeek, *context.mkaFile);
        if (MetaSeekSize == INVALID_FILEPOS_T)
        {
            // writing at the beginning failed, write at the end and provide a
            // short metaseek at the front
            context.metaSeek.Render(*context.mkaFile);
            lastElementEnd = context.metaSeek.GetEndPosition();

            KaxSeekHead ShortMetaSeek;
            ShortMetaSeek.EnableChecksum();
            ShortMetaSeek.IndexThis(context.metaSeek, context.fileSegment);
            MetaSeekSize = context.dummyStart.ReplaceWith(ShortMetaSeek, *context.mkaFile);
        }

        if (context.fileSegment.ForceSize(lastElementEnd - context.fileSegment.GetDataStart()))
        {
            context.fileSegment.OverwriteHead(*context.mkaFile);
        }

        // Finally, clean up
        context.project.reset();
    } catch (const libebml::CRTError&)
    {
        throw ExportException("libebml error");
    } catch (const std::bad_alloc&)
    {
        throw ExportException("Memory allocation error");
    }

    return ExportResult::Success;
}

///////////////////////////////////////////////////////////////////////////////

//// Export Plugin ////////////////////////////////////////////////////////////
class ExportMka final : public ExportPlugin
{
    public:
        ExportMka() = default;
        ~ExportMka() = default;

        int GetFormatCount() const override { return 1; };

        FormatInfo GetFormatInfo(int) const override {
            return { "Mka", XO("Matroska Files"), { "mka", "mkv" }, 255, true };
        };

        std::unique_ptr<ExportOptionsEditor> CreateOptionsEditor(
            int formatIndex, ExportOptionsEditor::Listener* listener
        ) const override {
            return std::make_unique<PlainExportOptionsEditor>(
                MkaOptions, listener
            );
        }

        std::vector<std::string> GetMimeTypes(int formatIndex) const override {
            return {"video/matroska", "audio/matroska"};
        }

        // TODO: Implement config parser
        bool ParseConfig(
            int, const rapidjson::Value& config,
            ExportProcessor::Parameters& parameters
        ) const override;

        // TODO: Implement export processor
        std::unique_ptr<ExportProcessor> CreateProcessor(int format) const override
        {
            return std::make_unique<MkaExportProcessor>();
        }
};

bool ExportMka::ParseConfig(
    int, const rapidjson::Value& config,
    ExportProcessor::Parameters& parameters
) const
{
    if (!config.IsObject() || !config.HasMember("bit_depth") || !config.HasMember("keep_labels") ||
        !config["bit_depth"].IsString() || !config["keep_labels"].IsBool())
    {
        return false;
    }

    const auto bitDepth = ExportValue(config["bit_depth"].GetString());
    const auto keepLabels = ExportValue(config["keep_labels"].GetBool());

    // Check to ensure the value of bit_depth is valid
    const auto& validFormats = MkaOptions.begin()[0].option.values;

    if (std::find(validFormats.begin(), validFormats.end(), bitDepth) == validFormats.end())
    {
        return false;
    }

    parameters = {
        { MkaOptionFormatID, bitDepth },
        { MkaOptionKeepLabelsID, keepLabels }
    };

    return true;
}

//// Miscellaneous ////////////////////////////////////////////////////////////

static ExportPluginRegistry::RegisteredPlugin sRegisteredPlugin{
    "Matroska", [] { return std::make_unique<ExportMka>(); }
};
