//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_APP_MAIN_NODESTORESCHEDULER_H_INCLUDED
#define RIPPLE_APP_MAIN_NODESTORESCHEDULER_H_INCLUDED

#include <divvy/nodestore/Scheduler.h>
#include <divvy/core/JobQueue.h>
#include <beast/threads/Stoppable.h>
#include <atomic>

namespace divvy {

/** A NodeStore::Scheduler which uses the JobQueue and implements the Stoppable API. */
class NodeStoreScheduler
    : public NodeStore::Scheduler
    , public beast::Stoppable
{
public:
    NodeStoreScheduler (Stoppable& parent);

    // VFALCO NOTE This is a temporary hack to solve the problem
    //             of circular dependency.
    //
    void setJobQueue (JobQueue& jobQueue);

    void onStop ();
    void onChildrenStopped ();
    void scheduleTask (NodeStore::Task& task);
    void onFetch (NodeStore::FetchReport const& report) override;
    void onBatchWrite (NodeStore::BatchWriteReport const& report) override;

private:
    void doTask (NodeStore::Task& task, Job&);

    JobQueue* m_jobQueue;
    std::atomic <int> m_taskCount;
};

} // divvy

#endif
