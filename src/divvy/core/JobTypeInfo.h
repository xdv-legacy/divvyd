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

#ifndef RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED
#define RIPPLE_CORE_JOBTYPEINFO_H_INCLUDED

namespace divvy
{

/** Holds all the 'static' information about a job, which does not change */
class JobTypeInfo
{
private:
    JobType const m_type;
    std::string const m_name;

    /** The limit on the number of running jobs for this job type. */
    int const m_limit;

    /** Can be skipped */
    bool const m_skip;

    /** Special jobs are not dispatched via the job queue */
    bool const m_special;

    /** Average and peak latencies for this job type. 0 is none specified */
    std::uint64_t const m_avgLatency;
    std::uint64_t const m_peakLatency;

public:
    // Not default constructible
    JobTypeInfo () = delete;

    JobTypeInfo (JobType type, std::string name, int limit,
            bool skip, bool special, std::uint64_t avgLatency, std::uint64_t peakLatency)
        : m_type (type)
        , m_name (name)
        , m_limit (limit)
        , m_skip (skip)
        , m_special (special)
        , m_avgLatency (avgLatency)
        , m_peakLatency (peakLatency)
    {

    }

    JobType type () const
    {
        return m_type;
    }

    std::string name () const
    {
        return m_name;
    }

    int limit () const
    {
        return m_limit;
    }

    bool skip () const
    {
        return m_skip;
    }

    bool special () const
    {
        return m_special;
    }

    std::uint64_t getAverageLatency () const
    {
        return m_avgLatency;
    }

    std::uint64_t getPeakLatency () const
    {
        return m_peakLatency;
    }
};

}

#endif
