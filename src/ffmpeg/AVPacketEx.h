/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  AVPacketEx.h

  Avery King split from FFmpeg.h

*******************************************************************************/
#ifndef __SAUCEDACITY_AVPACKETEX__
#define __SAUCEDACITY_AVPACKETEX__

#ifdef USE_FFMPEG
#include "FFmpeg.h"

/// Attach some C++ lifetime management to AVPacketEx, which owns some memory
/// resources.
struct AVPacketEx : public AVPacket
{
    AVPacketEx();

    AVPacketEx(const AVPacketEx &) = delete;
    AVPacketEx& operator= (const AVPacketEx&) = delete;

    AVPacketEx(AVPacketEx &&that);
    AVPacketEx &operator= (AVPacketEx &&that);

    ~AVPacketEx();

    void reset();

  private:
    void steal(AVPacketEx &&that);
};

#endif // end USE_FFMPEG

#endif // end __SAUCEDACITY_AVPACKETEX__
