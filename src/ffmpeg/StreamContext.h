/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  StreamContext.h

  Avery King split from FFmpeg.h

  This contains the definition for StreamContext and functions using
  StreamContext.

*******************************************************************************/
#ifndef __SAUCEDACITY_STREAMCONTEXT_H__
#define __SAUCEDACITY_STREAMCONTEXT_H__

#include "FFmpeg.h"
#include "AVPacketEx.h"

struct StreamContext
{
  bool                    m_use{};                           /// TRUE = this stream will be loaded into Saucedacity
  AVStream               *m_stream{};                        /// an AVStream *
  AVCodecContext         *m_codecCtx{};                      /// pointer to m_stream->codec

  Optional<AVPacketEx>    m_pkt;                          /// the last AVPacket we read for this stream
  uint8_t                *m_pktDataPtr{};                    /// pointer into m_pkt.data
  int                     m_pktRemainingSiz{};

  int64_t                 m_pts{};                           /// the current presentation time of the input stream
  int64_t                 m_ptsOffset{};                     /// packets associated with stream are relative to this

  int                     m_frameValid{};                    /// is m_decodedVideoFrame/m_decodedAudioSamples valid?
  AVMallocHolder<uint8_t> m_decodedAudioSamples;          /// decoded audio samples stored here
  unsigned int            m_decodedAudioSamplesSiz{};        /// current size of m_decodedAudioSamples
  size_t                  m_decodedAudioSamplesValidSiz{};   /// # valid bytes in m_decodedAudioSamples
  int                     m_initialchannels{};               /// number of channels allocated when we begin the importing. Assumes that number of channels doesn't change on the fly.

  size_t                  m_samplesize{};                     /// input sample size in bytes
  AVSampleFormat          m_samplefmt{ AV_SAMPLE_FMT_NONE  }; /// input sample format

  size_t                  m_osamplesize{};                   /// output sample size in bytes
  sampleFormat            m_osamplefmt{ floatSample };       /// output sample format

  StreamContext();
  ~StreamContext();
};

using Scs = ArrayOf<std::unique_ptr<StreamContext>>; /// StreamContext array typedef
using ScsPtr = std::shared_ptr<Scs>;                 /// Typedef of Scs array as a shared pointer

// functions
StreamContext *import_ffmpeg_read_next_frame(AVFormatContext* formatContext,
                                             StreamContext** streams,
                                             unsigned int numStreams);

#endif // end __SAUCEDACITY_STREAMCONTEXT_H__
