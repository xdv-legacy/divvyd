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

#ifndef RIPPLE_RPC_HANDLERS_LEDGER_H_INCLUDED
#define RIPPLE_RPC_HANDLERS_LEDGER_H_INCLUDED

#include <divvy/app/ledger/LedgerToJson.h>
#include <divvy/core/LoadFeeTrack.h>
#include <divvy/json/Object.h>
#include <divvy/server/Role.h>

namespace Json {
class Object;
}

namespace divvy {
namespace RPC {

// ledger [id|index|current|closed] [full]
// {
//    ledger: 'current' | 'closed' | <uint256> | <number>,  // optional
//    full: true | false    // optional, defaults to false.
// }

class LedgerHandler {
public:
    explicit LedgerHandler (Context&);

    Status check ();

    template <class Object>
    void writeResult (Object&);

    static const char* const name()
    {
        return "ledger";
    }

    static Role role()
    {
        return Role::USER;
    }

    static Condition condition()
    {
        return NO_CONDITION;
    }

private:
    Context& context_;
    Ledger::pointer ledger_;
    Json::Value result_;
    int options_ = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Implementation.

template <class Object>
void LedgerHandler::writeResult (Object& value)
{
    if (ledger_)
    {
        Json::copyFrom (value, result_);
        addJson (value, {*ledger_, options_, context_.yield});
    }
    else
    {
        auto& master = getApp().getLedgerMaster ();
        auto& yield = context_.yield;
        {
            auto&& closed = Json::addObject (value, jss::closed);
            addJson (closed, {*master.getClosedLedger(), 0, yield});
        }
        {
            auto&& open = Json::addObject (value, jss::open);
            addJson (open, {*master.getCurrentLedger(), 0, yield});
        }
    }
}

} // RPC
} // divvy

#endif
