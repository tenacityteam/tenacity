/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  StreamContext.h

  Avery King split from FFmpeg.h and FFmpeg.cpp

  The only reason why this file exists is just to pair with StreamContext.h for
  consistency.

*******************************************************************************/

#include "StreamContext.h"
#include <cstring>

extern void* memset(void* s, int c, size_t n);

StreamContext::StreamContext()
{
  memset(this, 0, sizeof(*this));
}

StreamContext::~StreamContext()
{
}

StreamContext *import_ffmpeg_read_next_frame(AVFormatContext* formatContext,
                                             StreamContext** streams,
                                             unsigned int numStreams)
{
   StreamContext *sc = NULL;
   AVPacketEx pkt;

   if (av_read_frame(formatContext, &pkt) < 0)
   {
      return NULL;
   }

   // Find a stream to which this frame belongs
   for (unsigned int i = 0; i < numStreams; i++)
   {
      if (streams[i]->m_stream->index == pkt.stream_index)
         sc = streams[i];
   }

   // Off-stream packet. Don't panic, just skip it.
   // When not all streams are selected for import this will happen very often.
   if (sc == NULL)
   {
      return (StreamContext*)1;
   }

   // Copy the frame to the stream context
   sc->m_pkt.emplace(std::move(pkt));

   sc->m_pktDataPtr = sc->m_pkt->data;
   sc->m_pktRemainingSiz = sc->m_pkt->size;

   return sc;
}
