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

#ifndef RIPPLE_APP_MISC_ACCOUNTSTATE_H_INCLUDED
#define RIPPLE_APP_MISC_ACCOUNTSTATE_H_INCLUDED

#include <divvy/basics/Blob.h>
#include <divvy/protocol/DivvyAddress.h>
#include <divvy/protocol/STAmount.h>
#include <divvy/protocol/STLedgerEntry.h>

namespace divvy {

//
// Provide abstract access to an account's state, such that
// access to the serialized format is hidden.
//

// VFALCO TODO Remove this class, its redundant and hardly used
class AccountState
{
public:
    // VFALCO TODO Figure out if we need this to be shared
    using pointer = std::shared_ptr<AccountState>;

    // For accounts in a ledger
    AccountState (std::shared_ptr<SLE const> sle,
        DivvyAddress const& naAccountI);

    SLE const&
    sle() const
    {
        return *mLedgerEntry;
    }

    void addJson (Json::Value& value);

private:
    // VFALCO TODO Remove this
    static std::string createGravatarUrl (uint128 uEmailHash);

    bool mValid = false;
    std::shared_ptr<SLE const> mLedgerEntry;
};

} // divvy

#endif
