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

#include <BeastConfig.h>

#ifndef NDEBUG
# define consistency_check(cond) bassert(cond)
#else
# define consistency_check(cond)
#endif

#include <divvy/peerfinder/impl/Bootcache.cpp>
#include <divvy/peerfinder/impl/PeerfinderConfig.cpp>
#include <divvy/peerfinder/impl/Endpoint.cpp>
#include <divvy/peerfinder/impl/Manager.cpp>
#include <divvy/peerfinder/impl/SlotImp.cpp>
#include <divvy/peerfinder/impl/SourceStrings.cpp>

#include <divvy/peerfinder/sim/GraphAlgorithms.h>
#include <divvy/peerfinder/sim/WrappedSink.h>
#include <divvy/peerfinder/sim/Predicates.h>
#include <divvy/peerfinder/sim/FunctionQueue.h>
#include <divvy/peerfinder/sim/Message.h>
#include <divvy/peerfinder/sim/NodeSnapshot.h>
#include <divvy/peerfinder/sim/Params.h>
#include <divvy/peerfinder/sim/Tests.cpp>

#include <divvy/peerfinder/tests/Livecache.test.cpp>
#include <divvy/peerfinder/tests/PeerFinder_test.cpp>

#if DOXYGEN
#include <divvy/peerfinder/README.md>
#endif
