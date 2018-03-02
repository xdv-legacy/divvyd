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
#include <divvy/app/tx/TransactionMaster.h>
#include <divvy/app/main/Application.h>
#include <divvy/basics/Log.h>
#include <divvy/basics/seconds_clock.h>

namespace divvy {

TransactionMaster::TransactionMaster ()
    : mCache ("TransactionCache", 65536, 1800, get_seconds_clock (),
        deprecatedLogs().journal("TaggedCache"))
{
}

bool TransactionMaster::inLedger (uint256 const& hash, std::uint32_t ledger)
{
    Transaction::pointer txn = mCache.fetch (hash);

    if (!txn)
        return false;

    txn->setStatus (COMMITTED, ledger);
    return true;
}

Transaction::pointer TransactionMaster::fetch (uint256 const& txnID, bool checkDisk)
{
    Transaction::pointer txn = mCache.fetch (txnID);

    if (!checkDisk || txn)
        return txn;

    txn = Transaction::load (txnID);

    if (!txn)
        return txn;

    mCache.canonicalize (txnID, txn);

    return txn;
}

STTx::pointer TransactionMaster::fetch (std::shared_ptr<SHAMapItem> const& item,
        SHAMapTreeNode::TNType type,
        bool checkDisk, std::uint32_t uCommitLedger)
{
    STTx::pointer  txn;
    Transaction::pointer            iTx = getApp().getMasterTransaction ().fetch (item->getTag (), false);

    if (!iTx)
    {

        if (type == SHAMapTreeNode::tnTRANSACTION_NM)
        {
            SerialIter sit (item->slice());
            txn = std::make_shared<STTx> (std::ref (sit));
        }
        else if (type == SHAMapTreeNode::tnTRANSACTION_MD)
        {
            Serializer s;
            int length;
            item->peekSerializer().getVL (s.modData (), 0, length);
            SerialIter sit (s.slice());

            txn = std::make_shared<STTx> (std::ref (sit));
        }
    }
    else
    {
        if (uCommitLedger)
            iTx->setStatus (COMMITTED, uCommitLedger);

        txn = iTx->getSTransaction ();
    }

    return txn;
}

bool TransactionMaster::canonicalize (Transaction::pointer* pTransaction)
{
    Transaction::pointer txn (*pTransaction);

    uint256 tid = txn->getID ();

    if (!tid)
        return false;

    // VFALCO NOTE canonicalize can change the value of txn!
    if (mCache.canonicalize (tid, txn))
    {
        *pTransaction = txn;
        return true;
    }

    // VFALCO NOTE I am unsure if this is necessary but better safe than sorry.
    *pTransaction = txn;
    return false;
}

void TransactionMaster::sweep (void)
{
    mCache.sweep ();
}

TaggedCache <uint256, Transaction>& TransactionMaster::getCache()
{
    return mCache;
}

} // divvy