/**********************************************************************

  Tenacity

  AudioMemoryManager.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "AudioMemoryManager.h"

AudioMemoryManager& AudioMemoryManager::Get()
{
    static AudioMemoryManager theMemoryManager;
    return theMemoryManager; // This reads too nicely.
}

void AudioMemoryManager::CreateBuffer(size_t size)
{
    for (auto& buffer : mBuffers)
    {
        if (buffer.second >= size && buffer.first.use_count() == 1)
        {
            return;
        }
    }

    mBuffers.emplace_back(new float[size], size);
}

std::shared_ptr<float> AudioMemoryManager::GetBuffer(size_t size)
{
    for (auto& buffer : mBuffers)
    {
        if (buffer.second >= size && buffer.first.use_count() == 1)
        {
            return buffer.first;
        }
    }

    // Couldn't find a suitable buffer. Return an empty one
    return {};
}
