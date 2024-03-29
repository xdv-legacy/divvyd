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
#include <divvy/app/tx/Transaction.h>
#include <divvy/basics/Log.h>
#include <divvy/core/DatabaseCon.h>
#include <divvy/app/ledger/LedgerMaster.h>
#include <divvy/app/main/Application.h>
#include <divvy/protocol/JsonFields.h>
#include <boost/optional.hpp>

namespace divvy {

Transaction::Transaction (STTx::ref sit, Validate validate, std::string& reason)
    noexcept
    : mTransaction (sit)
{
    try
    {
        mFromPubKey.setAccountPublic (mTransaction->getSigningPubKey ());
        mTransactionID  = mTransaction->getTransactionID ();
        mAccountFrom    = mTransaction->getSourceAccount ();
    }
    catch (std::exception& e)
    {
        reason = e.what();
        return;
    }
    catch (...)
    {
        reason = "Unexpected exception";
        return;
    }

    if (validate == Validate::NO ||
        (passesLocalChecks (*mTransaction, reason) && checkSign (reason)))
    {
        mStatus = NEW;
    }
}

Transaction::pointer Transaction::sharedTransaction (
    Blob const& vucTransaction, Validate validate)
{
    try
    {
        SerialIter sit (make_Slice(vucTransaction));
        std::string reason;

        return std::make_shared<Transaction> (std::make_shared<STTx> (sit),
            validate, reason);
    }
    catch (std::exception& e)
    {
        WriteLog(lsWARNING, Ledger) << "Exception constructing transaction" <<
            e.what ();
    }
    catch (...)
    {
        WriteLog(lsWARNING, Ledger) << "Exception constructing transaction";
    }

    return std::shared_ptr<Transaction> ();
}

//
// Misc.
//

bool Transaction::checkSign (std::string& reason) const
{
    if (! mFromPubKey.isValid ())
        reason = "Transaction has bad source public key";
    else if (! mTransaction->checkSign ())
        reason = "Transaction has bad signature";
    else
        return true;

    WriteLog (lsWARNING, Ledger) << reason;
    return false;
}

void Transaction::setStatus (TransStatus ts, std::uint32_t lseq)
{
    mStatus     = ts;
    mInLedger   = lseq;
}

TransStatus Transaction::sqlTransactionStatus(
    boost::optional<std::string> const& status)
{
    char const c = (status) ? (*status)[0] : TXN_SQL_UNKNOWN;

    switch (c)
    {
    case TXN_SQL_NEW:       return NEW;
    case TXN_SQL_CONFLICT:  return CONFLICTED;
    case TXN_SQL_HELD:      return HELD;
    case TXN_SQL_VALIDATED: return COMMITTED;
    case TXN_SQL_INCLUDED:  return INCLUDED;
    }

    assert (c == TXN_SQL_UNKNOWN);
    return INVALID;
}

Transaction::pointer Transaction::transactionFromSQL (
    boost::optional<std::uint64_t> const& ledgerSeq,
    boost::optional<std::string> const& status,
    Blob const& rawTxn,
    Validate validate)
{
    std::uint32_t const inLedger =
        rangeCheckedCast<std::uint32_t>(ledgerSeq.value_or (0));

    SerialIter it (make_Slice(rawTxn));
    auto txn = std::make_shared<STTx> (it);
    std::string reason;
    auto tr = std::make_shared<Transaction> (txn, validate, reason);

    tr->setStatus (sqlTransactionStatus (status));
    tr->setLedger (inLedger);
    return tr;
}

Transaction::pointer Transaction::load (uint256 const& id)
{
    std::string sql = "SELECT LedgerSeq,Status,RawTxn "
            "FROM Transactions WHERE TransID='";
    sql.append (to_string (id));
    sql.append ("';");

    boost::optional<std::uint64_t> ledgerSeq;
    boost::optional<std::string> status;
    Blob rawTxn;
    {
        auto db = getApp().getTxnDB ().checkoutDb ();
        soci::blob sociRawTxnBlob (*db);
        soci::indicator rti;

        *db << sql, soci::into (ledgerSeq), soci::into (status),
                soci::into (sociRawTxnBlob, rti);
        if (!db->got_data () || rti != soci::i_ok)
            return {};

        convert(sociRawTxnBlob, rawTxn);
    }

    return Transaction::transactionFromSQL (
        ledgerSeq, status, rawTxn, Validate::YES);
}

// options 1 to include the date of the transaction
Json::Value Transaction::getJson (int options, bool binary) const
{
    Json::Value ret (mTransaction->getJson (0, binary));

    if (mInLedger)
    {
        ret[jss::inLedger] = mInLedger;        // Deprecated.
        ret[jss::ledger_index] = mInLedger;

        if (options == 1)
        {
            auto ledger = getApp().getLedgerMaster ().
                    getLedgerBySeq (mInLedger);
            if (ledger)
                ret[jss::date] = ledger->getCloseTimeNC ();
        }
    }

    return ret;
}

} // divvy
