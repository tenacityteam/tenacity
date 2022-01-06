/*******************************************************************************

  Saucedacity: A Digital Audio Editor

  AVPacketEx.cpp

  Avery King split from FFmpeg.h

*******************************************************************************/
#ifdef USE_FFMPEG

#include "AVPacketEx.h"

AVPacketEx::AVPacketEx()
{
  av_init_packet(this);
  data = nullptr;
  size = 0;
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
  // This does not deallocate the pointer, but it frees side data.
  av_free_packet(this);
}

void AVPacketEx::steal(AVPacketEx &&that)
{
  memcpy(this, &that, sizeof(that));
  av_init_packet(&that);
  that.data = nullptr;
  that.size = 0;
}

#endif // end USE_FFMPEG
