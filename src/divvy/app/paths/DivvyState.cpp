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
#include <divvy/app/main/Application.h>
#include <divvy/app/paths/DivvyState.h>
#include <divvy/protocol/STAmount.h>
#include <cstdint>
#include <beast/cxx14/memory.h> // <memory>

namespace divvy {

DivvyState::pointer
DivvyState::makeItem (
    AccountID const& accountID,
        std::shared_ptr<SLE const> sle)
{
    // VFALCO Does this ever happen in practice?
    if (! sle || sle->getType () != ltRIPPLE_STATE)
        return {};
    return std::make_shared<DivvyState>(
        std::move(sle), accountID);
}

DivvyState::DivvyState (
    std::shared_ptr<SLE const>&& sle,
        AccountID const& viewAccount)
    : mLedgerEntry (std::move(sle))
    , mLowLimit (mLedgerEntry->getFieldAmount (sfLowLimit))
    , mHighLimit (mLedgerEntry->getFieldAmount (sfHighLimit))
    , mLowID (mLowLimit.getIssuer ())
    , mHighID (mHighLimit.getIssuer ())
    , mBalance (mLedgerEntry->getFieldAmount (sfBalance))
{
    mFlags          = mLedgerEntry->getFieldU32 (sfFlags);

    mLowQualityIn   = mLedgerEntry->getFieldU32 (sfLowQualityIn);
    mLowQualityOut  = mLedgerEntry->getFieldU32 (sfLowQualityOut);

    mHighQualityIn  = mLedgerEntry->getFieldU32 (sfHighQualityIn);
    mHighQualityOut = mLedgerEntry->getFieldU32 (sfHighQualityOut);

    mViewLowest = (mLowID == viewAccount);

    if (!mViewLowest)
        mBalance.negate ();
}

Json::Value DivvyState::getJson (int)
{
    Json::Value ret (Json::objectValue);
    ret["low_id"] = to_string (mLowID);
    ret["high_id"] = to_string (mHighID);
    return ret;
}

std::vector <DivvyState::pointer>
getDivvyStateItems (
    AccountID const& accountID,
    Ledger::ref ledger)
{
    std::vector <DivvyState::pointer> items;
    forEachItem(*ledger, accountID, getApp().getSLECache(),
        [&items,&accountID](
        std::shared_ptr<SLE const> const&sleCur)
        {
             auto ret = DivvyState::makeItem (accountID, sleCur);
             if (ret)
                items.push_back (ret);
        });

    return items;
}

} // divvy
