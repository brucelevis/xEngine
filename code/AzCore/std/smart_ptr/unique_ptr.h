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
#ifndef AZSTD_SMART_PTR_UNIQUE_PTR_H
#define AZSTD_SMART_PTR_UNIQUE_PTR_H

#include <AzCore/std/smart_ptr/sp_convertible.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/extent.h>
#include <AzCore/std/typetraits/remove_extent.h>
#include <AzCore/std/utils.h>
#include <AzCore/Memory/Memory.h>
#include <memory>

namespace AZStd
{
    /// 20.7.12.2 unique_ptr for single objects.
    template <typename T, typename Deleter = std::default_delete<T> >
    using unique_ptr = std::unique_ptr<T, Deleter>;

    template<class T>
    struct hash;

    template <typename T, typename Deleter>
    struct hash<unique_ptr<T, Deleter>>
    {
        typedef unique_ptr<T, Deleter> argument_type;
        typedef AZStd::size_t result_type;
        inline result_type operator()(const argument_type& value) const
        {
            return std::hash<argument_type>()(value);
        }
    };

    template<typename T, typename... Args>
    AZStd::enable_if_t<!AZStd::is_array<T>::value && AZ::HasAZClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return AZStd::unique_ptr<T>(aznew T(AZStd::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    AZStd::enable_if_t<!AZStd::is_array<T>::value && !AZ::HasAZClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return AZStd::unique_ptr<T>(new T(AZStd::forward<Args>(args)...));
    }

    // The reason that there is not an array aznew version version of make_unique is because AZClassAllocator does not support array new
    template<typename T>
    AZStd::enable_if_t<AZStd::is_array<T>::value && AZStd::extent<T>::value == 0, unique_ptr<T>> make_unique(std::size_t size)
    {
        return AZStd::unique_ptr<T>(new typename AZStd::remove_extent<T>::type[size]());
    }

    template<typename T, typename... Args>
    AZStd::enable_if_t<AZStd::is_array<T>::value && AZStd::extent<T>::value != 0, unique_ptr<T>> make_unique(Args&&... args) = delete;

} // namespace AZStd

namespace std
{
    //! VS2013 HACK: This specializations only occurs in VS2013 because std::is_copy_constructible always returns true.
    //! https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
    template<typename T, typename Deleter>
    struct is_copy_constructible<std::unique_ptr<T, Deleter>>
        : public std::false_type
    {};
}

namespace AZ
{
    AZ_INTERNAL_VARIATION_SPECIALIZATION_2_CONCAT_1(T, Deleter, AZStd::unique_ptr, "AZStd::unique_ptr<", ">", "{B55F90DA-C21E-4EB4-9857-87BE6529BA6D}");
}

#endif // AZSTD_SMART_PTR_UNIQUE_PTR_H
#pragma once
