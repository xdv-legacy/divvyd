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

#ifndef RIPPLE_APP_PATHS_RIPPLELINECACHE_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLELINECACHE_H_INCLUDED

#include <divvy/app/paths/DivvyState.h>
#include <divvy/basics/hardened_hash.h>
#include <cstddef>
#include <memory>
#include <vector>

namespace divvy {

// Used by Pathfinder
class DivvyLineCache
{
public:
    using DivvyStateVector = std::vector <DivvyState::pointer>;
    using pointer = std::shared_ptr <DivvyLineCache>;
    using ref = pointer const&;

    explicit DivvyLineCache (Ledger::ref l);

    Ledger::ref getLedger () // VFALCO TODO const?
    {
        return mLedger;
    }

    std::vector<DivvyState::pointer> const&
    getDivvyLines (AccountID const& accountID);

private:
    using LockType = DivvyMutex;
    using ScopedLockType = std::lock_guard <LockType>;
    LockType mLock;

    divvy::hardened_hash<> hasher_;
    Ledger::pointer mLedger;

    struct AccountKey
    {
        AccountID account_;
        std::size_t hash_value_;

        AccountKey (AccountID const& account, std::size_t hash)
            : account_ (account)
            , hash_value_ (hash)
        { }

        AccountKey (AccountKey const& other) = default;

        AccountKey&
        operator=(AccountKey const& other) = default;

        bool operator== (AccountKey const& lhs) const
        {
            return hash_value_ == lhs.hash_value_ && account_ == lhs.account_;
        }

        std::size_t
        get_hash () const
        {
            return hash_value_;
        }

        struct Hash
        {
            std::size_t
            operator () (AccountKey const& key) const noexcept
            {
                return key.get_hash ();
            }
        };
    };

    hash_map <AccountKey, DivvyStateVector, AccountKey::Hash> mRLMap;
};

} // divvy

#endif
