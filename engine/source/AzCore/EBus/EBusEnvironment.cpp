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
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        ContextBase::ContextBase()
        {
            // When we create Global context we store addition information for access 
            // to the TLS Environment and context index to support EBusEnvironmnets
            // hold a reference to the variable (even though we will never release it)
            static EnvironmentVariable<Internal::EBusEnvironmentTLSAccessors> s_tlsAccessor = Environment::CreateVariable<Internal::EBusEnvironmentTLSAccessors>(Internal::EBusEnvironmentTLSAccessors::GetId());

            m_ebusEnvironmentTLSIndex = s_tlsAccessor->m_numUniqueEBuses++;
            m_ebusEnvironmentGetter = s_tlsAccessor->m_getter;
        }

        //////////////////////////////////////////////////////////////////////////
        ContextBase::ContextBase(EBusEnvironment* environment)
            : m_ebusEnvironmentTLSIndex(size_t(-1))
            , m_ebusEnvironmentGetter(nullptr)
        {
            // we need m_ebusEnvironmentGetter and m_ebusEnvironmentTLSIndex only in the global context
            (void)environment;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ_THREAD_LOCAL EBusEnvironment* EBusEnvironmentTLSAccessors::s_tlsCurrentEnvironment = nullptr;

        //////////////////////////////////////////////////////////////////////////
        EBusEnvironmentTLSAccessors::EBusEnvironmentTLSAccessors()
            : m_getter(&GetTLSEnvironment)
            , m_setter(&SetTLSEnvironment)
            , m_numUniqueEBuses(0)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        u32 EBusEnvironmentTLSAccessors::GetId()
        {
            return AZ_CRC("EBusEnvironmentTLSAccessors", 0x2fe98c39);
        }

        //////////////////////////////////////////////////////////////////////////
        EBusEnvironment* EBusEnvironmentTLSAccessors::GetTLSEnvironment()
        {
            return s_tlsCurrentEnvironment;
        }

        //////////////////////////////////////////////////////////////////////////
        void EBusEnvironmentTLSAccessors::SetTLSEnvironment(EBusEnvironment* environment)
        {
            s_tlsCurrentEnvironment = environment;
        }

    } // namespace Internal

    //////////////////////////////////////////////////////////////////////////
    EBusEnvironment::EBusEnvironment()
    {
        m_tlsAccessor = Environment::CreateVariable<Internal::EBusEnvironmentTLSAccessors>(Internal::EBusEnvironmentTLSAccessors::GetId());

#ifdef AZ_DEBUG_BUILD
        m_stackPrevEnvironment = reinterpret_cast<EBusEnvironment*>(AZ_INVALID_POINTER);
#endif // AZ_DEBUG_BUILD
    }

    //////////////////////////////////////////////////////////////////////////
    EBusEnvironment::~EBusEnvironment()
    {
#ifdef AZ_DEBUG_BUILD
        AZ_Assert(m_stackPrevEnvironment == reinterpret_cast<EBusEnvironment*>(AZ_INVALID_POINTER), "You can't destroy BusEvironment %p while it's active, make sure you PopBusEnvironment when not in use!", this);
#endif // AZ_DEBUG_BUILD

        if (!m_busContexts.empty())
        {
            for (auto& busContextIsOwnerPair : m_busContexts)
            {
                if (busContextIsOwnerPair.second)
                {
                    busContextIsOwnerPair.first->~ContextBase();
                    AZ_OS_FREE(busContextIsOwnerPair.first);
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void EBusEnvironment::ActivateOnCurrentThread()
    {
#ifdef AZ_DEBUG_BUILD
        AZ_Assert(m_stackPrevEnvironment == reinterpret_cast<EBusEnvironment*>(AZ_INVALID_POINTER), "Environment %p is already active on another thread. This is illegal!", this);
#endif // AZ_DEBUG_BUILD

#if !defined(AZ_PLATFORM_APPLE) // thread_local not supported prior to iOS9 
        m_stackPrevEnvironment = m_tlsAccessor->m_getter();
        m_tlsAccessor->m_setter(this);
#else
        AZ_Error("System", false, "EBus environments are currently NOT support on Apple platforms! This is true until our minimal spec supports thread_local variables!");
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    void EBusEnvironment::DeactivateOnCurrentThread()
    {
#ifdef AZ_DEBUG_BUILD
        AZ_Assert(m_stackPrevEnvironment != reinterpret_cast<EBusEnvironment*>(AZ_INVALID_POINTER), "Environment %p is not active you can't call Deactivate!", this);
#endif // AZ_DEBUG_BUILD

#if !defined(AZ_PLATFORM_APPLE) // thread_local not supported prior to iOS9 
        m_tlsAccessor->m_setter(m_stackPrevEnvironment);
#else
        AZ_Error("System", false, "EBus environments are currently NOT support on Apple platforms! This is true until our minimal spec supports thread_local variables!");
#endif // 

#ifdef AZ_DEBUG_BUILD
        m_stackPrevEnvironment = reinterpret_cast<EBusEnvironment*>(AZ_INVALID_POINTER);
#endif // AZ_DEBUG_BUILD
    }

    //////////////////////////////////////////////////////////////////////////
    Internal::ContextBase* EBusEnvironment::FindContext(size_t tlsKey)
    {
        if (tlsKey >= m_busContexts.size())
        {
            return nullptr;
        }
        return m_busContexts[tlsKey].first;
    }

    //////////////////////////////////////////////////////////////////////////
    bool EBusEnvironment::InsertContext(size_t tlsKey, Internal::ContextBase* context, bool isTakeOwnership)
    {
        if (tlsKey >= m_busContexts.size())
        {
            m_busContexts.resize(tlsKey+1, AZStd::make_pair(nullptr,false));
        }
        
        if (m_busContexts[tlsKey].first != nullptr)
        {
            return false; // we already context at this key
        }

        m_busContexts[tlsKey] = AZStd::make_pair(context, isTakeOwnership);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    EBusEnvironment* EBusEnvironment::Create()
    {
        return new(AZ_OS_MALLOC(sizeof(EBusEnvironment), AZStd::alignment_of<EBusEnvironment>::value)) EBusEnvironment;
    }

    //////////////////////////////////////////////////////////////////////////
    void EBusEnvironment::Destroy(EBusEnvironment* environment)
    {
        if (environment)
        {
            environment->~EBusEnvironment();
            AZ_OS_FREE(environment);
        }
    }

} // namespace AZ
