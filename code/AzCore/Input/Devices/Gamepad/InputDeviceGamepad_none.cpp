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

#if !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_APPLE) && !defined(AZ_PLATFORM_WINDOWS)

#include <AzCore/Input/Devices/Gamepad/InputDeviceGamepad.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceGamepad::Implementation* InputDeviceGamepad::Implementation::Create(InputDeviceGamepad&)
    {
        return nullptr;
    }
} // namespace AZ

#endif // !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_APPLE) && !defined(AZ_PLATFORM_WINDOWS)