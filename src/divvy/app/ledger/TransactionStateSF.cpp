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
#include <divvy/app/ledger/TransactionStateSF.h>
#include <divvy/app/main/Application.h>
#include <divvy/app/misc/NetworkOPs.h>
#include <divvy/app/tx/TransactionMaster.h>
#include <divvy/nodestore/Database.h>
#include <divvy/protocol/HashPrefix.h>

namespace divvy {

TransactionStateSF::TransactionStateSF()
{
}

// VFALCO This might be better as Blob&&
void TransactionStateSF::gotNode (bool fromFilter,
                                  SHAMapNodeID const& id,
                                  uint256 const& nodeHash,
                                  Blob& nodeData,
                                  SHAMapTreeNode::TNType type)
{
    // VFALCO SHAMapSync filters should be passed the SHAMap, the
    //        SHAMap should provide an accessor to get the injected Database,
    //        and this should use that Database instad of getNodeStore
    assert(type !=
        SHAMapTreeNode::tnTRANSACTION_NM);
    getApp().getNodeStore().store(
        hotTRANSACTION_NODE,
            std::move (nodeData), nodeHash);
}

bool TransactionStateSF::haveNode (SHAMapNodeID const& id,
                                   uint256 const& nodeHash,
                                   Blob& nodeData)
{
    return getApp().getOPs ().getFetchPack (nodeHash, nodeData);
}

} // divvy
