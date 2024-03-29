//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2014 Ripple Labs Inc.

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
#include <divvy/app/tx/impl/Transactor.h>
#include <divvy/basics/Log.h>
#include <divvy/protocol/Indexes.h>

namespace divvy {

class CreateTicket
    : public Transactor
{
public:
    CreateTicket (
        STTx const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("CreateTicket"))
    {

    }

    TER
    preCheck () override
    {
        if (mTxn.isFieldPresent (sfExpiration))
        {
            if (mTxn.getFieldU32 (sfExpiration) == 0)
            {
                m_journal.warning <<
                    "Malformed transaction: bad expiration";
                return temBAD_EXPIRATION;
            }
        }

        return Transactor::preCheck ();
    }

    /** Returns the reserve the account would have if an offer was added. */
    STAmount
    getAccountReserve (SLE::pointer account)
    {
        return STAmount (mEngine->getLedger ()->getReserve (
            account->getFieldU32 (sfOwnerCount) + 1));
    }

    TER doApply () override
    {
        assert (mTxnAccount);

        // A ticket counts against the reserve of the issuing account, but we
        // check the starting balance because we want to allow dipping into the
        // reserve to pay fees.
        if (mPriorBalance < getAccountReserve (mTxnAccount))
            return tecINSUFFICIENT_RESERVE;

        std::uint32_t expiration (0);

        if (mTxn.isFieldPresent (sfExpiration))
        {
            expiration = mTxn.getFieldU32 (sfExpiration);

            if (mEngine->getLedger ()->getParentCloseTimeNC () >= expiration)
                return tesSUCCESS;
        }

        SLE::pointer sleTicket = std::make_shared<SLE>(ltTICKET,
            getTicketIndex (mTxnAccountID, mTxn.getSequence ()));
        sleTicket->setFieldAccount (sfAccount, mTxnAccountID);
        sleTicket->setFieldU32 (sfSequence, mTxn.getSequence ());
        if (expiration != 0)
            sleTicket->setFieldU32 (sfExpiration, expiration);
        mEngine->view().entryCreate (sleTicket);

        if (mTxn.isFieldPresent (sfTarget))
        {
            AccountID const target_account (mTxn.getFieldAccount160 (sfTarget));

            SLE::pointer sleTarget = mEngine->view().entryCache (ltACCOUNT_ROOT,
                getAccountRootIndex (target_account));

            // Destination account does not exist.
            if (!sleTarget)
                return tecNO_TARGET;

            // The issuing account is the default account to which the ticket
            // applies so don't bother saving it if that's what's specified.
            if (target_account != mTxnAccountID)
                sleTicket->setFieldAccount (sfTarget, target_account);
        }

        std::uint64_t hint;

        auto describer = [&](SLE::pointer p, bool b)
        {
            Ledger::ownerDirDescriber(p, b, mTxnAccountID);
        };

        TER result = mEngine->view ().dirAdd (
            hint,
            getOwnerDirIndex (mTxnAccountID),
            sleTicket->getIndex (),
            describer);

        if (m_journal.trace) m_journal.trace <<
            "Creating ticket " << to_string (sleTicket->getIndex ()) <<
            ": " << transHuman (result);

        if (result != tesSUCCESS)
            return result;

        sleTicket->setFieldU64(sfOwnerNode, hint);

        // If we succeeded, the new entry counts agains the creator's reserve.
        adjustOwnerCount(mEngine->view(), mTxnAccount, 1);

        return result;
    }
};

TER
transact_CreateTicket (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    if (! engine->enableTickets())
        return temDISABLED;
    return CreateTicket (txn, params, engine).apply ();
}

}
