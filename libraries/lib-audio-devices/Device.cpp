/**********************************************************************

  Tenacity

  Device.cpp

  Avery King

  License: GPL v2 or later

**********************************************************************/

#include "Device.h"

Device::Device() : mDeviceType{Device::Type::Null}, mName{"(null)"},
                   mHostName{"(null)"}, mDeviceIndex{-1}, mHostIndex{-1},
                   mNumChannels{-1}, mDefaultDevice{false}
{
}

Device::Device(const Device& other)
{
    mDeviceType    = other.mDeviceType;
    mName          = other.mName;
    mHostName      = other.mHostName;
    mDeviceIndex   = other.mDeviceIndex;
    mHostIndex     = other.mHostIndex;
    mNumChannels   = other.mNumChannels;
    mDefaultDevice = other.mDefaultDevice;
}

Device::Device(Device&& other)
{
    mDeviceType    = other.mDeviceType;
    mName          = other.mName;
    mHostName      = other.mHostName;
    mDeviceIndex   = other.mDeviceIndex;
    mHostIndex     = other.mHostIndex;
    mNumChannels   = other.mNumChannels;
    mDefaultDevice = other.mDefaultDevice;

    other.Reset();
}

Device::operator bool() const
{
    return mDeviceType  != Device::Type::Null || mName != "(null)"   ||
           mHostName    != "(null)"           || mDeviceIndex  != -1 ||
           mNumChannels != -1;
}

void Device::SetDeviceType(Type type) noexcept
{
    mDeviceType = type;
}

void Device::SetName(const std::string& name) noexcept
{
    mName = name;
}

void Device::SetHostName(const std::string& hostName) noexcept
{
    mHostName = hostName;
}

void Device::SetNumChannels(int channels) noexcept
{
    mNumChannels = channels;
}

void Device::SetDeviceIndex(int index) noexcept
{
    mDeviceIndex = index;
}

void Device::SetHostIndex(int hostIndex) noexcept
{
    mHostIndex = hostIndex;
}

Device::Type Device::GetDeviceType() const
{
    return mDeviceType;
}

std::string Device::GetName() const
{
    return mName;
}

std::string Device::GetHostName() const
{
    return mHostName;
}

int Device::GetNumChannels() const
{
    return mNumChannels;
}

int Device::GetDeviceIndex() const
{
    return mDeviceIndex;
}

int Device::GetHostIndex() const
{
    return mHostIndex;
}

void Device::SetDefaultDevice(bool isDefault) noexcept
{
    mDefaultDevice = isDefault;
}

bool Device::IsDefaultDevice() const noexcept
{
    return mDefaultDevice;
}

void Device::Reset() noexcept
{
    mDeviceType = Device::Type::Null;
    mName          = "(null)";
    mHostName      = "(null)";
    mDeviceIndex   = -1;
    mHostIndex     = -1;
    mNumChannels   = -1;
    mDefaultDevice = false;
}

