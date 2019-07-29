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

#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for input channels that emit two dimensional axis input values.
    //! Example: game-pad thumb-stick x and y
    class InputChannelAxis2D : public InputChannel
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelAxis2D, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom data struct for two dimensional axis data
        struct AxisData2D : public InputChannel::CustomData
        {
            AZ_RTTI(AxisData2D, "{AA0FF4D4-ED98-4AEE-A3AB-B442287E2B7B}");
            ~AxisData2D() override = default;

            AZ::Vector2 m_values = AZ::Vector2::CreateZero();
            AZ::Vector2 m_deltas = AZ::Vector2::CreateZero();
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel being constructed
        //! \param[in] inputDevice Input device that owns the input channel
        explicit InputChannelAxis2D(const InputChannelId& inputChannelId,
                                    const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannelAxis2D);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelAxis2D() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the distance from the origin (length of the vector formed by the axis values)
        //! \return The distance from the origin (length of the vector formed by the axis values)
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the total distance moved since last frame
        //! \return The total distance moved since last frame
        float GetDelta() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the two dimensional axis data associated with the input channel
        //! \return Pointer to the two dimensional axis data
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
        //! \param[in] rawValues The raw axis values to process
        void ProcessRawInputEvent(const AZ::Vector2& rawValues);

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AxisData2D m_axisData2D;    //!< Current two dimensional axis values of the input channel
    };
} // namespace AZ
