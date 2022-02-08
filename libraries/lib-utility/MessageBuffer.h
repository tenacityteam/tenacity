/**********************************************************************

  Tenacity: A Digital Audio Editor

  MessageBuffer.h

  Avery King split from AudioIO.h

**********************************************************************/

#pragma once

#include "MemoryX.h"
#include <atomic>

//! Communicate data atomically from one writer thread to one reader.
/*!
 This is not a queue: it is not necessary for each write to be read.
 Rather loss of a message is allowed:  writer may overwrite.
 Data must be default-constructible and reassignable.
 */
template<typename Data>
class MessageBuffer
{
    struct UpdateSlot
    {
        std::atomic<bool> mBusy{ false };
        Data mData;
    };

    NonInterfering<UpdateSlot> mSlots[2];

    std::atomic<unsigned char> mLastWrittenSlot{ 0 };

    public:
        void Initialize()
        {
            for (auto &slot : mSlots)
            {
                // Lock both slots first, maybe spinning a little
                while ( slot.mBusy.exchange( true, std::memory_order_acquire ) )
                {
                }
            }

            mSlots[0].mData = {};
            mSlots[1].mData = {};
            mLastWrittenSlot.store( 0, std::memory_order_relaxed );

            for (auto &slot : mSlots)
            {
                slot.mBusy.exchange( false, std::memory_order_release );
            }
        }

        // Move data out (if available), or else copy it out
        Data Read()
        {
            // Whichever slot was last written, prefer to read that.
            auto idx = mLastWrittenSlot.load( std::memory_order_relaxed );
            idx = 1 - idx;
            bool wasBusy = false;
            do {
                // This loop is unlikely to execute twice, but it might because the
                // producer thread is writing a slot.
                idx = 1 - idx;
                wasBusy = mSlots[idx].mBusy.exchange( true, std::memory_order_acquire );
            } while ( wasBusy );

            // Copy the slot
            auto result = std::move( mSlots[idx].mData );

            mSlots[idx].mBusy.store( false, std::memory_order_release );

            return result;
        }

        //! Reassign a slot by move or copy
        template<typename Arg = Data&&>
        void Write( Arg &&arg )
        {
            // Whichever slot was last written, prefer to write the other.
            auto idx = mLastWrittenSlot.load( std::memory_order_relaxed );
            bool wasBusy = false;
            do {
                // This loop is unlikely to execute twice, but it might because the
                // consumer thread is reading a slot.
                idx = 1 - idx;
                wasBusy = mSlots[idx].mBusy.exchange( true, std::memory_order_acquire );
            } while ( wasBusy );

            mSlots[idx].mData = std::forward<Arg>(arg);
            mLastWrittenSlot.store( idx, std::memory_order_relaxed );

            mSlots[idx].mBusy.store( false, std::memory_order_release );
        }
};
