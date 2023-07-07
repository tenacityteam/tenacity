/**********************************************************************

  Tenacity

  AudioMemoryManager.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <utility>

/// A singleton class to manage audio buffers.
class AUDIO_DEVICES_API AudioMemoryManager final
{
    private:
        AudioMemoryManager() = default;
        ~AudioMemoryManager() = default;

        std::vector<std::pair<std::shared_ptr<float>, size_t>> mBuffers;

    public:
        static AudioMemoryManager& Get();

        /** @brief Allocates a new buffer.
         * 
         * CreateBuffer() allocates a new buffer of exactly `size`.
         * 
         * @note If an existing buffer with the same size exists **and** it is
         * currently unused, CreateBuffer() will not do anything.
         * 
         */
        void CreateBuffer(size_t size);

        /** @brief Returns a buffer of a specified size.
         * 
         * GetBuffer() returns a buffer that is at least `size`. If previous
         * buffers were allocated, AudioMemoryManager will return one of those
         * buffers instead if they are at least `size`. The buffers may exceed
         * `size` if needed.
         * 
         * @note GetBuffer() does not allocatee new buffers. If there is no
         * buffer that is at least `size` available, an empty `shared_ptr` is
         * returned. See @ref CreateBuffer() for more info.
         * 
         */
        std::shared_ptr<float> GetBuffer(size_t size);
};
