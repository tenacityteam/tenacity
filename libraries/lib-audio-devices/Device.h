/**********************************************************************

  Tenacity

  Device.h

  Avery King

  License: GPL v2 or later

**********************************************************************/

#pragma once

#include <string>
#include <any>

/** @brief Represents an input or output device in Tenacity.
 * 
 * This is modified from DeviceSourceMap in Tenacity. This is basically a
 * glorified version of that class apart from the addition of device types.
 * 
 */
class AUDIO_DEVICES_API Device
{
    public:
        enum class Type
        {
            Input,
            Output,
            Null,   /// The Device object is invalid
            Dummy   /// Not a real device, but still valid
        };

    private:
        Type mDeviceType; /// Device type.
        std::string mName; /// Name of the device, previously deviceString.
        std::string mHostName; /// Name of the host, previously hostString.
        int mDeviceIndex; /// PortAudio device index.
        int mHostIndex; /// PortAudio host index for the device.
        int mNumChannels; /// Number of input or output channels.
        bool mDefaultDevice; /// If this device is the default device.

    public:
        Device();
        ~Device() = default;

        Device(const Device& other);
        Device(Device&& other);

        operator bool() const;

        /** @brief Sets the device type.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetDeviceType(Type type) noexcept;

        /** @brief Sets the device name.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetName(const std::string& name) noexcept;

        /** @brief Sets the device's host name.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetHostName(const std::string& hostName) noexcept;

        /** @brief Sets the device's number of channels.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetNumChannels(int channels) noexcept;

        /** @brief Sets the device's PortAudio device index.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetDeviceIndex(int index) noexcept;

        /** @brief Sets the device's PortAudio host index.
         * 
         * @warning This function is only meant for DeviceManager or unless
         * you're constructing a dummy device. Only use it if you know what
         * you're doing!
         * 
        */
        void SetHostIndex(int hostIndex) noexcept;

        /// Returns the device's type. 
        Type GetDeviceType() const;

        /// Returns the device's name.
        std::string GetName() const;

        /// Return's the host name of the current device.
        std::string GetHostName() const;

        /// Returns the number of channels for the device.
        int GetNumChannels() const;

        /// Returns the device's PortAudio device index.
        int GetDeviceIndex() const;

        /// Returns the device's PortAudio host index.
        int GetHostIndex() const;

        void SetDefaultDevice(bool isDefault) noexcept;

        /// Returns if this device is the default device. 
        bool IsDefaultDevice() const noexcept;

	/// Resets the device info, reverting back to an invalid device.
	void Reset() noexcept;
};
