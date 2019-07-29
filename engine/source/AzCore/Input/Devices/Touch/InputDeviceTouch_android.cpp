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

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_ANDROID)

#include <AzCore/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzCore/Input/Buses/Notifications/RawInputNotificationBus_android.h>

#include <AzCore/Android/Utils.h>

#include <AzCore/std/parallel/mutex.h>

#include <android/input.h>
#include <android/keycodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for android touch input devices
    class InputDeviceTouchAndroid : public InputDeviceTouch::Implementation
                                  , public RawInputNotificationBusAndroid::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceTouchAndroid, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceTouchAndroid(InputDeviceTouch& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceTouchAndroid() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::InputDeviceTouch::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::InputDeviceTouch::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::RawInputNotificationsAndroid::OnRawInputEvent
        void OnRawInputEvent(const AInputEvent* rawInputEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw input events on android can (and likely will) be dispatched from a thread other than
        //! main, so we can't immediately call InputDeviceTouch::Implementation::QueueRawTouchEvent.
        //! Instead, we'll store them in m_threadAwareRawTouchEvents then process in TickInputDevice
        //! on the main thread, ensuring all access is locked using m_threadAwareRawTouchEventsMutex.
        ///@{
        AZStd::vector<RawTouchEvent> m_threadAwareRawTouchEvents;
        AZStd::mutex                 m_threadAwareRawTouchEventsMutex;
        ///@}
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation* InputDeviceTouch::Implementation::Create(InputDeviceTouch& inputDevice)
    {
        return aznew InputDeviceTouchAndroid(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchAndroid::InputDeviceTouchAndroid(InputDeviceTouch& inputDevice)
        : InputDeviceTouch::Implementation(inputDevice)
        , m_threadAwareRawTouchEvents()
        , m_threadAwareRawTouchEventsMutex()
    {
        RawInputNotificationBusAndroid::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouchAndroid::~InputDeviceTouchAndroid()
    {
        RawInputNotificationBusAndroid::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouchAndroid::IsConnected() const
    {
        // Touch input is always available on android
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchAndroid::TickInputDevice()
    {
        // The input event loop is pumped by another thread on android so all raw input events this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        // Because m_threadAwareRawTouchEvents is updated from another thread, but also needs to be
        // processed and cleared in this function, swapping it with an empty vector kills two birds
        // with one stone in a very efficient and elegant manner (kudos to scottr for the idea).
        AZStd::vector<RawTouchEvent> rawTouchEvents;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTouchEventsMutex);
            rawTouchEvents.swap(m_threadAwareRawTouchEvents);
        }

        // Queue the raw touch events that were received over the last frame
        for (const RawTouchEvent& rawTouchEvent : rawTouchEvents)
        {
            QueueRawTouchEvent(rawTouchEvent);
        }

        // Process the raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouchAndroid::OnRawInputEvent(const AInputEvent* rawInputEvent)
    {
        // Reference: https://developer.android.com/ndk/reference/group___input.html

        // Discard non-touch events
        const int eventType = AInputEvent_getType(rawInputEvent);
        if (eventType != AINPUT_EVENT_TYPE_MOTION)
        {
            return;
        }

        // Get the action code and pointer index
        const int action = AMotionEvent_getAction(rawInputEvent);
        const int actionCode = (action & AMOTION_EVENT_ACTION_MASK);
        const int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        float pressure = AMotionEvent_getPressure(rawInputEvent, pointerIndex);

        // Get the touch event state
        RawTouchEvent::State rawTouchState = RawTouchEvent::State::Began;
        switch (actionCode)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {
                rawTouchState = RawTouchEvent::State::Began;
            }
            break;
            case AMOTION_EVENT_ACTION_MOVE:
            {
                rawTouchState = RawTouchEvent::State::Moved;
            }
            break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
            case AMOTION_EVENT_ACTION_CANCEL:
            {
                rawTouchState = RawTouchEvent::State::Ended;
                pressure = 0.0f;
            }
            break;
            default:
            {
                return;
            }
            break;
        }

        // Get the rest of the touch event data
        const float x = AMotionEvent_getX(rawInputEvent, pointerIndex);
        const float y = AMotionEvent_getY(rawInputEvent, pointerIndex);
        const int pointerId = AMotionEvent_getPointerId(rawInputEvent, pointerIndex);

        // normalize the touch location
        int windowWidth = 0;
        int windowHeight = 0;
        if (!AZ::Android::Utils::GetWindowSize(windowWidth, windowHeight))
        {
            AZ_ErrorOnce("AndroidTouch", false, "Unable to get the window size, touch input may not behave correctly.");
            windowWidth = 1;
            windowHeight = 1;
        }

        // Push the raw touch event onto the thread safe queue for processing in TickInputDevice
        const RawTouchEvent rawTouchEvent(x / static_cast<float>(windowWidth),
                                          y / static_cast<float>(windowHeight),
                                          pressure,
                                          pointerId,
                                          rawTouchState);
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTouchEventsMutex);
        m_threadAwareRawTouchEvents.push_back(rawTouchEvent);
    }
}

#endif // defined(AZ_PLATFORM_ANDROID)
