/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Input/Channels/InputChannel.h>

#include <AzCore/Math/Quaternion.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit quaternion input values.
    //! Example: motion sensor data (orientation)
    class InputChannelQuaternion : public InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelQuaternion, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom data struct for three dimensional axis data
        struct QuaternionData : public InputChannel::CustomData
        {
            AZ_RTTI(QuaternionData, "{ABR4447B-34C6-9D17-B4E8-5B62109C15EA}");
            ~QuaternionData() override = default;

            AZ::Quaternion m_value = AZ::Quaternion::CreateIdentity();
            AZ::Quaternion m_delta = AZ::Quaternion::CreateIdentity();
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        InputChannelQuaternion(const InputChannelId& inputChannelId,
                               const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelQuaternion);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelQuaternion() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Quaternion input channels do not represent a single value
        //! \return 0.0f
        float GetValue() const override { return 0.0f; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Quaternion input channels do not represent a single value
        //! \return 0.0f
        float GetDelta() const override { return 0.0f; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the quaternion data associated with the input channel
        //! \return Pointer to the quaternion data
        const InputChannel::CustomData* GetCustomData() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::InputChannelRequests::ResetState
        void ResetState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Process a raw input event, that will in turn update the channel's state based on whether
        //! it's active/engaged or inactive/idle, broadcasting an input event if the channel is left
        //! in a non-idle state. This function (or InputChannel::UpdateState) should only be called
        //! a max of once per channel per frame from InputDeviceRequests::TickInputDevice to ensure
        //! that input channels broadcast no more than one event each frame (and at the same time).
        //! \param[in] rawValues The raw quaternion value to process
        void ProcessRawInputEvent(const AZ::Quaternion& rawValue);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        QuaternionData m_quaternionData; //!< Current quaternion value of the input channel
    };
} // namespace AZ
