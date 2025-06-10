// SPDX-License-Identifier: GPL-2.0-or-later
/**********************************************************************

  Tenacity: A Digital Audio Editor

  ExportMka.cpp

  Steve Lhomme

**********************************************************************/

#include "ExportOptionsEditor.h"
#include "ExportPlugin.h"
#include "ExportPluginRegistry.h"
#include "PlainExportOptionsEditor.h"
#include "Tags.h"

#include <memory>

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
        // bool ParseConfig(
        //     int formatIndex, const rapidjson::Value& config,
        //     ExportProcessor::Parameters& parameters
        // ) const override;

        // TODO: Implement export processor
        std::unique_ptr<ExportProcessor> CreateProcessor(int format) const override { return nullptr; }
};

static ExportPluginRegistry::RegisteredPlugin sRegisteredPlugin{
    "Matroska", [] { return std::make_unique<ExportMka>(); }
};
