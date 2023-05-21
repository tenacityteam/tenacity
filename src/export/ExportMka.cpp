// SPDX-License-Identifier: GPL-2.0-or-later
/**********************************************************************

  Tenacity: A Digital Audio Editor

  ExportMka.cpp

  Steve Lhomme

**********************************************************************/

#ifdef USE_LIBMATROSKA

#include "Export.h"
#include "../Tags.h"
#include "../Track.h"

// Tenacity libraries
#include <lib-files/wxFileNameWrapper.h>
#include <lib-string-utils/CodeConversions.h>
#include <lib-project-rate/ProjectRate.h>

#include "TenacityHeaders.h"

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
using namespace Tenacity;

class ExportMKAOptions final : public wxPanelWrapper
{
public:

    ExportMKAOptions(wxWindow *parent, int format);
    virtual ~ExportMKAOptions();

    void PopulateOrExchange(ShuttleGui & S);
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};

ExportMKAOptions::ExportMKAOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
    ShuttleGui S(this, eIsCreatingFromPrefs);
    PopulateOrExchange(S);

    TransferDataToWindow();
}

ExportMKAOptions::~ExportMKAOptions()
{
    TransferDataFromWindow();
}

ChoiceSetting MKAFormat {
    wxT("/FileFormats/MKAFormat"),
    {
        ByColumns,
        {
          XO("PCM 16-bit (Little Endian)") ,
          XO("PCM 24-bit (Little Endian)") ,
          XO("PCM Float 32-bit") ,
#ifdef USE_LIBFLAC
          XO("FLAC 16-bit"),
          XO("FLAC 24-bit"),
#endif
        },
        {
          wxT("16"),
          wxT("24"),
          wxT("f32"),
#ifdef USE_LIBFLAC
          wxT("flac16"),
          wxT("flac24"),
#endif
        }
    },
};

void ExportMKAOptions::PopulateOrExchange(ShuttleGui & S)
{
    S.StartVerticalLay();
    {
        S.StartHorizontalLay(wxCENTER);
        {
            S.StartMultiColumn(2, wxCENTER);
            {
                S.TieChoice( XXO("Bit depth:"), MKAFormat);
                // TODO select OK of the dialog by default ?
                // TODO select the language to use for strings (app, und, or a list ?)
                // TODO select the emphasis type
                S.TieCheckBox(XXO("Keep Labels"), {wxT("/FileFormats/MkaExportLabels"), true});
            }
            S.EndMultiColumn();
        }
        S.EndHorizontalLay();
    }
    S.EndVerticalLay();

    return;
}

bool ExportMKAOptions::TransferDataToWindow()
{
    return true;
}

bool ExportMKAOptions::TransferDataFromWindow()
{
    ShuttleGui S(this, eIsSavingToPrefs);
    PopulateOrExchange(S);

    gPrefs->Flush();

    return true;
}


class ExportMka final : public ExportPlugin
{
public:
    ExportMka();

    void OptionsCreate(ShuttleGui &S, int format) override;

    ProgressResult Export(TenacityProject *project,
                                std::unique_ptr<ProgressDialog> &pDialog,
                                unsigned channels,
                                const wxFileNameWrapper &fName,
                                bool selectedOnly,
                                double t0,
                                double t1,
                                MixerSpec *mixerSpec,
                                const Tags *metadata,
                                int subformat) override;
};

ExportMka::ExportMka()
:  ExportPlugin()
{
    std::srand(std::time(nullptr));
    AddFormat();
    SetFormat(wxT("MKA"),0);
    AddExtension(wxT("mka"),0);
    SetCanMetaData(true,0);
    SetDescription(XO("Matroska Audio Files"),0);
}

void ExportMka::OptionsCreate(ShuttleGui &S, int format)
{
    S.AddWindow( safenew ExportMKAOptions{ S.GetParent(), format } );
}

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
    ::FLAC__StreamEncoderInitStatus init()
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

ProgressResult ExportMka::Export(TenacityProject *project,
                                std::unique_ptr<ProgressDialog> &pDialog,
                                unsigned numChannels,
                                const wxFileNameWrapper &fName,
                                bool selectionOnly,
                                double t0,
                                double t1,
                                MixerSpec *mixerSpec,
                                const Tags *metadata,
                                int subformat)
{
    auto bitDepthPref = MKAFormat.Read();
    auto updateResult = ProgressResult::Success;

    const wxString url = fName.GetAbsolutePath(wxString(), wxPATH_NATIVE);
    try
    {
        StdIOCallback mka_file(url, ::MODE_CREATE);

        InitProgress( pDialog, fName,
            selectionOnly
                ? XO("Exporting the selected audio as MKA")
                : XO("Exporting the audio as MKA") );
        auto &progress = *pDialog;

        EbmlHead FileHead;
        (EbmlString &) GetChild<EDocType>(FileHead) = "matroska";
#if LIBMATROSKA_VERSION >= 0x010406
        (EbmlUInteger &) GetChild<EDocTypeVersion>(FileHead) = 4; // needed for LanguageBCP47
#else
        (EbmlUInteger &) GetChild<EDocTypeVersion>(FileHead) = 2;
#endif
        (EbmlUInteger &) GetChild<EDocTypeReadVersion>(FileHead) = 2; // needed for SimpleBlock
        (EbmlUInteger &) GetChild<EMaxIdLength>(FileHead) = 4;
        (EbmlUInteger &) GetChild<EMaxSizeLength>(FileHead) = 8;
        FileHead.Render(mka_file, true);

        KaxSegment FileSegment;
        auto SegmentSize = FileSegment.WriteHead(mka_file, 5);

        // reserve some space for the Meta Seek writen at the end
        EbmlVoid DummyStart;
        DummyStart.SetSize(128);
        DummyStart.Render(mka_file);

        KaxSeekHead MetaSeek;
        MetaSeek.EnableChecksum();

        const auto &tracks = TrackList::Get( *project );
        const double rate = ProjectRate::Get( *project ).GetRate();
        const uint64 TIMESTAMP_UNIT = std::llround(UINT64_C(1000000000) / rate);
        // const uint64 TIMESTAMP_UNIT = 1000000; // 1 ms

        EbmlMaster & MyInfos = GetChild<KaxInfo>(FileSegment);
        MyInfos.EnableChecksum();
        (EbmlFloat &) GetChild<KaxDuration>(MyInfos) = (t1 - t0) * UINT64_C(1000000000) / TIMESTAMP_UNIT; // in TIMESTAMP_UNIT
        GetChild<KaxDuration>(MyInfos).SetPrecision(EbmlFloat::FLOAT_64);
        (EbmlUnicodeString &) GetChild<KaxMuxingApp>(MyInfos)  = ToWString(std::string("libebml ") + EbmlCodeVersion + std::string(" + libmatroska ") + KaxCodeVersion);
        (EbmlUnicodeString &) GetChild<KaxWritingApp>(MyInfos) = ToWString(APP_NAME) + L" " + AUDACITY_VERSION_STRING;
        (EbmlUInteger &) GetChild<KaxTimecodeScale>(MyInfos) = TIMESTAMP_UNIT;
        GetChild<KaxDateUTC>(MyInfos).SetEpochDate(time(nullptr));
        binary SegUID[16];
        FillRandomUUID(SegUID);
        GetChild<KaxSegmentUID>(MyInfos).CopyBuffer(SegUID, 16);
        filepos_t InfoSize = MyInfos.Render(mka_file);
        if (InfoSize != 0)
            MetaSeek.IndexThis(MyInfos, FileSegment);

        KaxTracks & MyTracks = GetChild<KaxTracks>(FileSegment);
        MyTracks.EnableChecksum();

        sampleFormat format;
        uint64 bytesPerSample;
        const char *codecID;
        bool outInterleaved;
        if (bitDepthPref == wxT("24"))
        {
            codecID = "A_PCM/INT/LIT";
            format = int24Sample;
            bytesPerSample = 3 * numChannels;
            outInterleaved = true;
        }
        else if (bitDepthPref == wxT("16"))
        {
            codecID = "A_PCM/INT/LIT";
            format = int16Sample;
            bytesPerSample = 2 * numChannels;
            outInterleaved = true;
        }
        else if (bitDepthPref == wxT("f32"))
        {
            codecID = "A_PCM/FLOAT/IEEE";
            format = floatSample;
            bytesPerSample = 4 * numChannels;
            outInterleaved = true;
        }
#ifdef USE_LIBFLAC
        else if (bitDepthPref == wxT("flac16"))
        {
            codecID = "A_FLAC";
            format = int16Sample;
            bytesPerSample = 2 * numChannels;
            outInterleaved = false;
        }
        else if (bitDepthPref == wxT("flac24"))
        {
            codecID = "A_FLAC";
            format = int24Sample;
            bytesPerSample = 3 * numChannels;
            outInterleaved = false;
        }
#endif

        // TODO support multiple tracks
        KaxTrackEntry & MyTrack1 = GetChild<KaxTrackEntry>(MyTracks);
        MyTrack1.SetGlobalTimecodeScale(TIMESTAMP_UNIT);

        (EbmlUInteger &) GetChild<KaxTrackType>(MyTrack1) = MATROSKA_TRACK_TYPE_AUDIO;
        (EbmlUInteger &) GetChild<KaxTrackNumber>(MyTrack1) = 1;
        (EbmlUInteger &) GetChild<KaxTrackUID>(MyTrack1) = GetRandomUID64();
        (EbmlUInteger &) GetChild<KaxTrackDefaultDuration>(MyTrack1) = MS_PER_FRAME * 1000000;
        (EbmlString &) GetChild<KaxTrackLanguage>(MyTrack1) = "und";
#if LIBMATROSKA_VERSION >= 0x010406
        (EbmlString &) GetChild<KaxLanguageIETF>(MyTrack1) = "und";
#endif
        auto waveTracks = tracks.Selected< const WaveTrack >();
        auto pT = waveTracks.begin();
        if (*pT)
        {
            const auto sTrackName = (*pT)->GetName();
            if (!sTrackName.empty() && sTrackName != (*pT)->GetDefaultName())
                (EbmlUnicodeString &) GetChild<KaxTrackName>(MyTrack1) = (UTFstring)sTrackName;
        }

        EbmlMaster & MyTrack1Audio = GetChild<KaxTrackAudio>(MyTrack1);
        (EbmlFloat &) GetChild<KaxAudioSamplingFreq>(MyTrack1Audio) = rate;
        (EbmlUInteger &) GetChild<KaxAudioChannels>(MyTrack1Audio) = numChannels;
        (EbmlString &) GetChild<KaxCodecID>(MyTrack1) = codecID;
        switch(format)
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
        }
#ifdef USE_LIBFLAC
        MkaFLACEncoder encoder;
        if (bitDepthPref == wxT("flac16") || bitDepthPref == wxT("flac24"))
        {
            encoder.set_bits_per_sample(format == int24Sample ? 24 : 16);

            encoder.set_channels(numChannels) &&
            encoder.set_sample_rate(lrint(rate));
            auto status = encoder.init();
            if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
            {
                throw new std::runtime_error("toto");
            }
            const auto & buf = encoder.GetInitBuffer();
            GetChild<KaxCodecPrivate>(MyTrack1).CopyBuffer(buf.data(), buf.size());
        }
#endif
        filepos_t TrackSize = MyTracks.Render(mka_file);
        if (TrackSize != 0)
            MetaSeek.IndexThis(MyTracks, FileSegment);

        // reserve some space after the track (to match mkvmerge for now)
        // EbmlVoid DummyTrack;
        // DummyTrack.SetSize(1068);
        // DummyTrack.Render(mka_file);

        // add tags
        KaxTags & Tags = GetChild<KaxTags>(FileSegment);
        Tags.EnableChecksum();
        if (metadata == nullptr)
            metadata = &Tags::Get( *project );
        KaxTag *prevTag = nullptr;
        SetMetadata(metadata, prevTag, Tags, TAG_TITLE,     MATROSKA_TARGET_TYPE_TRACK, L"TITLE");
        SetMetadata(metadata, prevTag, Tags, TAG_GENRE,     MATROSKA_TARGET_TYPE_TRACK, L"GENRE");
        SetMetadata(metadata, prevTag, Tags, TAG_ARTIST,    MATROSKA_TARGET_TYPE_ALBUM, L"ARTIST");
        SetMetadata(metadata, prevTag, Tags, TAG_ALBUM,     MATROSKA_TARGET_TYPE_ALBUM, L"TITLE");
        SetMetadata(metadata, prevTag, Tags, TAG_TRACK,     MATROSKA_TARGET_TYPE_ALBUM, L"PART_NUMBER");
        SetMetadata(metadata, prevTag, Tags, TAG_YEAR,      MATROSKA_TARGET_TYPE_ALBUM, L"DATE_RELEASED");
        SetMetadata(metadata, prevTag, Tags, TAG_COMMENTS,  MATROSKA_TARGET_TYPE_ALBUM, L"COMMENT");
        SetMetadata(metadata, prevTag, Tags, TAG_COPYRIGHT, MATROSKA_TARGET_TYPE_ALBUM, L"COPYRIGHT");
        filepos_t TagsSize = Tags.Render(mka_file);
        if (TagsSize != 0)
            MetaSeek.IndexThis(Tags, FileSegment);

        KaxCues AllCues;
        AllCues.SetGlobalTimecodeScale(TIMESTAMP_UNIT);
        AllCues.EnableChecksum();

        const size_t maxFrameSamples = MS_PER_FRAME * rate / 1000; // match mkvmerge
        const auto SAMPLES_PER_RUN = maxFrameSamples * bytesPerSample;

        auto mixer = CreateMixer(tracks, selectionOnly,
                                 t0, t1,
                                 numChannels, SAMPLES_PER_RUN, outInterleaved,
                                 rate, format, mixerSpec);

        // add clusters
        std::unique_ptr<ClusterMuxer> Muxer;
#ifdef USE_LIBFLAC
        ArraysOf<FLAC__int32> splitBuff{ numChannels, SAMPLES_PER_RUN, true };
#endif

        ClusterMuxer::MuxerTime prevTime{0, 0, TIMESTAMP_UNIT, rate};
        while (updateResult == ProgressResult::Success)
        {
            auto samplesThisRun = mixer->Process(SAMPLES_PER_RUN);
            if (samplesThisRun == 0)
            {
#ifdef USE_LIBFLAC
                if (bitDepthPref == wxT("flac16") || bitDepthPref == wxT("flac24"))
                {
                    if (!Muxer)
                    {
                        Muxer = std::make_unique<ClusterMuxer>(FileSegment, MyTrack1, AllCues, prevTime);
                    }
                    encoder.finish();
#if 0 // doesn't work and is not necessary
                    if (encoder.hasNewCodecPrivate())
                    {
                        const auto & buf = encoder.GetInitBuffer();
                        KaxCodecPrivate &replacePrivate = GetChild<KaxCodecPrivate>(MyTrack1);
                        if (buf.size() && replacePrivate.GetSize())
                        {
                            replacePrivate.CopyBuffer(buf.data(), buf.size());
                            MyTracks.OverwriteData(mka_file);
                        }
                    }
#endif
                }
#endif
                if (Muxer)
                {
                    prevTime = Muxer->Finish(mka_file, MetaSeek);
                    Muxer = nullptr;
                }
                break; //finished
            }

            if (!Muxer)
            {
                Muxer = std::make_unique<ClusterMuxer>(FileSegment, MyTrack1, AllCues, prevTime);
            }

#ifdef USE_LIBFLAC
            if (bitDepthPref == wxT("flac16") || bitDepthPref == wxT("flac24"))
            {
                for (size_t i = 0; i < numChannels; i++)
                {
                    samplePtr mixed = mixer->GetBuffer(i);
                    if (format == int24Sample) {
                        for (decltype(samplesThisRun) j = 0; j < samplesThisRun; j++)
                            splitBuff[i][j] = ((int *)mixed)[j];
                    }
                    else {
                        for (decltype(samplesThisRun) j = 0; j < samplesThisRun; j++)
                            splitBuff[i][j] = ((short *)mixed)[j];
                    }
                }
                auto b = reinterpret_cast<FLAC__int32**>( splitBuff.get() );
                if (encoder.Process(Muxer, b, samplesThisRun))
                {
                    prevTime = Muxer->Finish(mka_file, MetaSeek);
                    Muxer = nullptr;
                }
            }
            else
#endif
            {
                samplePtr mixed = mixer->GetBuffer();
                DataBuffer *dataBuff = new DataBuffer((binary*)mixed, samplesThisRun * bytesPerSample, nullptr, true);
                if (Muxer->AddBuffer(*dataBuff, samplesThisRun))
                {
                    prevTime = Muxer->Finish(mka_file, MetaSeek);
                    Muxer = nullptr;
                }
            }

            updateResult = progress.Update(mixer->MixGetCurrentTime() - t0, t1 - t0);
        }

        // add cues
        filepos_t CueSize = AllCues.Render(mka_file);
        if (CueSize != 0)
            MetaSeek.IndexThis(AllCues, FileSegment);

        uint64 lastElementEnd = AllCues.GetEndPosition();

        // add markers as chapters
        if (gPrefs->Read(wxT("/FileFormats/MkaExportLabels"), true))
        {
            const auto trackRange = tracks.Any<const LabelTrack>();
            if (!trackRange.empty())
            {
                KaxChapters & EditionList = GetChild<KaxChapters>(FileSegment);
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
                filepos_t ChaptersSize = EditionList.Render(mka_file);
                if (ChaptersSize != 0)
                {
                    MetaSeek.IndexThis(EditionList, FileSegment);
                    lastElementEnd = EditionList.GetEndPosition();
                }
            }
        }

        auto MetaSeekSize = DummyStart.ReplaceWith(MetaSeek, mka_file);
        if (MetaSeekSize == INVALID_FILEPOS_T)
        {
            // writing at the beginning failed, write at the end and provide a
            // short metaseek at the front
            MetaSeek.Render(mka_file);
            lastElementEnd = MetaSeek.GetEndPosition();

            KaxSeekHead ShortMetaSeek;
            ShortMetaSeek.EnableChecksum();
            ShortMetaSeek.IndexThis(MetaSeek, FileSegment);
            MetaSeekSize = DummyStart.ReplaceWith(ShortMetaSeek, mka_file);
        }

        if (FileSegment.ForceSize(lastElementEnd - FileSegment.GetDataStart()))
        {
            FileSegment.OverwriteHead(mka_file);
        }

    } catch (const libebml::CRTError &) {
        updateResult = ProgressResult::Failed;
    } catch (const std::bad_alloc &) {
        updateResult = ProgressResult::Failed;
    }

    return updateResult;
}

static Exporter::RegisteredExportPlugin sRegisteredPlugin{ "Matroska",
    []{ return std::make_unique< ExportMka >(); }
};

#endif // USE_LIBMATROSKA
