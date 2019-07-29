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
#ifndef AZCORE_JOBS_JOBCOMPLETION_SPIN_H
#define AZCORE_JOBS_JOBCOMPLETION_SPIN_H 1

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/parallel/spin_mutex.h>

namespace AZ
{
    /**
     * Job which allows caller to block until it is completed using spin lock.
     * This should be in special cases where the jobs are super short and semaphore
     * creation is too expensive.
     * IMPORTANT: Don't use JobCompletionSpin by default, use JobCompletion! This class
     * is provided only for special cases. You should always verify with profiling that
     * is it actually faster when you use it, as spins (mutex or jobs) introduce a whole
     * new set of problems.
     */
    class JobCompletionSpin
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobCompletionSpin, ThreadPoolAllocator, 0)

        JobCompletionSpin(JobContext* context = nullptr)
            : Job(false, context)
            , m_mutex(true) //locked initially
        {
        }

        /**
         * Call this function to start the job and block until the job has been completed.
         */
        void StartAndWaitForCompletion()
        {
            Start();

            // Wait until we can lock it again.
            m_mutex.lock();
        }

        virtual void Reset(bool isClearDependent)
        {
            Job::Reset(isClearDependent);

            // make sure the mutex is locked
            m_mutex.try_lock();
        }

    protected:
        virtual void Process()
        {
            //unlock the mutex, allowing WaitForCompletion to lock it and continue. Note that this can be
            //called before this job has even been started.
            m_mutex.unlock();
        }

        AZStd::spin_mutex m_mutex;
    };
}

#endif
#pragma once