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
#include <divvy/app/ledger/ConsensusTransSetSF.h>
#include <divvy/app/main/Application.h>
#include <divvy/app/misc/NetworkOPs.h>
#include <divvy/app/tx/TransactionMaster.h>
#include <divvy/basics/Log.h>
#include <divvy/basics/SHA512Half.h>
#include <divvy/core/JobQueue.h>
#include <divvy/nodestore/Database.h>
#include <divvy/protocol/HashPrefix.h>

namespace divvy {

ConsensusTransSetSF::ConsensusTransSetSF (NodeCache& nodeCache)
    : m_nodeCache (nodeCache)
{
}

void ConsensusTransSetSF::gotNode (bool fromFilter, const SHAMapNodeID& id, uint256 const& nodeHash,
                                   Blob& nodeData, SHAMapTreeNode::TNType type)
{
    if (fromFilter)
        return;

    m_nodeCache.insert (nodeHash, nodeData);

    if ((type == SHAMapTreeNode::tnTRANSACTION_NM) && (nodeData.size () > 16))
    {
        // this is a transaction, and we didn't have it
        WriteLog (lsDEBUG, TransactionAcquire) << "Node on our acquiring TX set is TXN we may not have";

        try
        {
            // skip prefix
            Serializer s (nodeData.data() + 4, nodeData.size() - 4);
            SerialIter sit (s.slice());
            STTx::pointer stx = std::make_shared<STTx> (std::ref (sit));
            assert (stx->getTransactionID () == nodeHash);
            getApp().getJobQueue ().addJob (
                jtTRANSACTION, "TXS->TXN",
                std::bind (&NetworkOPs::submitTransaction, &getApp().getOPs (),
                           std::placeholders::_1, stx));
        }
        catch (...)
        {
            WriteLog (lsWARNING, TransactionAcquire) << "Fetched invalid transaction in proposed set";
        }
    }
}

bool ConsensusTransSetSF::haveNode (const SHAMapNodeID& id, uint256 const& nodeHash,
                                    Blob& nodeData)
{
    if (m_nodeCache.retrieve (nodeHash, nodeData))
        return true;

    // VFALCO TODO Use a dependency injection here
    Transaction::pointer txn = getApp().getMasterTransaction().fetch(nodeHash, false);

    if (txn)
    {
        // this is a transaction, and we have it
        WriteLog (lsTRACE, TransactionAcquire) << "Node in our acquiring TX set is TXN we have";
        Serializer s;
        s.add32 (HashPrefix::transactionID);
        txn->getSTransaction ()->add (s);
        assert(sha512Half(s.slice()) == nodeHash);
        nodeData = s.peekData ();
        return true;
    }

    return false;
}

} // divvy
