/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  AVPacketEx.cpp

  Avery King split from FFmpeg.h

*******************************************************************************/
#ifdef USE_FFMPEG

#include "AVPacketEx.h"
#include <stdexcept>

AVPacketEx::AVPacketEx()
{
  int ok = av_new_packet(this, 0);
  if (ok != 0)
  {
    throw std::bad_alloc();
  }
}

AVPacketEx::AVPacketEx(AVPacketEx &&that)
{
  steal(std::move(that));
}

AVPacketEx& AVPacketEx::operator= (AVPacketEx &&that)
{
  if (this != &that)
  {
    reset();
    steal(std::move(that));
  }

  return *this;
}

AVPacketEx::~AVPacketEx()
{
  reset();
}

void AVPacketEx::reset()
{
  av_packet_unref(this);
}

void AVPacketEx::steal(AVPacketEx &&that)
{
  av_packet_move_ref(this, &that);
}

#endif // end USE_FFMPEG
