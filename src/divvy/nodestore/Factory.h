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

#ifndef RIPPLE_NODESTORE_FACTORY_H_INCLUDED
#define RIPPLE_NODESTORE_FACTORY_H_INCLUDED

#include <divvy/nodestore/Backend.h>
#include <divvy/nodestore/Scheduler.h>
#include <beast/utility/Journal.h>

namespace divvy {
namespace NodeStore {

/** Base class for backend factories. */
class Factory
{
public:
    virtual
    ~Factory() = default;

    /** Retrieve the name of this factory. */
    virtual
    std::string
    getName() const = 0;

    /** Create an instance of this factory's backend.

        @param keyBytes The fixed number of bytes per key.
        @param keyValues A set of key/value configuration pairs.
        @param scheduler The scheduler to use for running tasks.
        @return A pointer to the Backend object.
    */
    virtual
    std::unique_ptr <Backend>
    createInstance (size_t keyBytes, Section const& parameters,
        Scheduler& scheduler, beast::Journal journal) = 0;
};

}
}

#endif
