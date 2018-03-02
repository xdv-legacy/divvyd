//------------------------------------------------------------------------------
/*
    This file is part of divvyd: https://github.com/xdv/divvyd
    Copyright (c) 2012, 2013 Divvy Labs Inc.

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

// This has to be included early to prevent an obscure MSVC compile error
#include <boost/asio/deadline_timer.hpp>

#include <divvy/protocol/JsonFields.h>

#include <divvy/rpc/RPCHandler.h>

#include <divvy/rpc/impl/Coroutine.cpp>
#include <divvy/rpc/impl/Manager.cpp>
#include <divvy/rpc/impl/RPCHandler.cpp>
#include <divvy/rpc/impl/Status.cpp>
#include <divvy/rpc/impl/Yield.cpp>
#include <divvy/rpc/impl/Utilities.cpp>

#include <divvy/rpc/handlers/Handlers.h>
#include <divvy/rpc/handlers/AccountCurrencies.cpp>
#include <divvy/rpc/handlers/AccountInfo.cpp>
#include <divvy/rpc/handlers/AccountLines.cpp>
#include <divvy/rpc/handlers/AccountObjects.cpp>
#include <divvy/rpc/handlers/AccountOffers.cpp>
#include <divvy/rpc/handlers/AccountTx.cpp>
#include <divvy/rpc/handlers/AccountTxOld.cpp>
#include <divvy/rpc/handlers/AccountTxSwitch.cpp>
#include <divvy/rpc/handlers/BlackList.cpp>
#include <divvy/rpc/handlers/BookOffers.cpp>
#include <divvy/rpc/handlers/CanDelete.cpp>
#include <divvy/rpc/handlers/Connect.cpp>
#include <divvy/rpc/handlers/ConsensusInfo.cpp>
#include <divvy/rpc/handlers/Feature.cpp>
#include <divvy/rpc/handlers/FetchInfo.cpp>
#include <divvy/rpc/handlers/GatewayBalances.cpp>
#include <divvy/rpc/handlers/GetCounts.cpp>
#include <divvy/rpc/handlers/Internal.cpp>
#include <divvy/rpc/handlers/Ledger.cpp>
#include <divvy/rpc/handlers/LedgerAccept.cpp>
#include <divvy/rpc/handlers/LedgerCleaner.cpp>
#include <divvy/rpc/handlers/LedgerClosed.cpp>
#include <divvy/rpc/handlers/LedgerCurrent.cpp>
#include <divvy/rpc/handlers/LedgerData.cpp>
#include <divvy/rpc/handlers/LedgerEntry.cpp>
#include <divvy/rpc/handlers/LedgerHeader.cpp>
#include <divvy/rpc/handlers/LedgerRequest.cpp>
#include <divvy/rpc/handlers/LogLevel.cpp>
#include <divvy/rpc/handlers/LogRotate.cpp>
#include <divvy/rpc/handlers/NoDivvyCheck.cpp>
#include <divvy/rpc/handlers/OwnerInfo.cpp>
#include <divvy/rpc/handlers/PathFind.cpp>
#include <divvy/rpc/handlers/Peers.cpp>
#include <divvy/rpc/handlers/Ping.cpp>
#include <divvy/rpc/handlers/Print.cpp>
#include <divvy/rpc/handlers/Random.cpp>
#include <divvy/rpc/handlers/DivvyPathFind.cpp>
#include <divvy/rpc/handlers/ServerInfo.cpp>
#include <divvy/rpc/handlers/ServerState.cpp>
#include <divvy/rpc/handlers/Sign.cpp>
#include <divvy/rpc/handlers/SignFor.cpp>
#include <divvy/rpc/handlers/Stop.cpp>
#include <divvy/rpc/handlers/Submit.cpp>
#include <divvy/rpc/handlers/SubmitMultiSigned.cpp>
#include <divvy/rpc/handlers/Subscribe.cpp>
#include <divvy/rpc/handlers/TransactionEntry.cpp>
#include <divvy/rpc/handlers/Tx.cpp>
#include <divvy/rpc/handlers/TxHistory.cpp>
#include <divvy/rpc/handlers/UnlAdd.cpp>
#include <divvy/rpc/handlers/UnlDelete.cpp>
#include <divvy/rpc/handlers/UnlList.cpp>
#include <divvy/rpc/handlers/UnlLoad.cpp>
#include <divvy/rpc/handlers/UnlNetwork.cpp>
#include <divvy/rpc/handlers/UnlReset.cpp>
#include <divvy/rpc/handlers/UnlScore.cpp>
#include <divvy/rpc/handlers/Unsubscribe.cpp>
#include <divvy/rpc/handlers/ValidationCreate.cpp>
#include <divvy/rpc/handlers/ValidationSeed.cpp>
#include <divvy/rpc/handlers/WalletPropose.cpp>
#include <divvy/rpc/handlers/WalletSeed.cpp>

#include <divvy/rpc/impl/AccountFromString.cpp>
#include <divvy/rpc/impl/Accounts.cpp>
#include <divvy/rpc/impl/GetAccountObjects.cpp>
#include <divvy/rpc/impl/Handler.cpp>
#include <divvy/rpc/impl/KeypairForSignature.cpp>
#include <divvy/rpc/impl/LegacyPathFind.cpp>
#include <divvy/rpc/impl/LookupLedger.cpp>
#include <divvy/rpc/impl/ParseAccountIds.cpp>
#include <divvy/rpc/impl/TransactionSign.cpp>
#include <divvy/rpc/impl/RPCVersion.cpp>

#include <divvy/rpc/tests/Coroutine.test.cpp>
#include <divvy/rpc/tests/JSONRPC.test.cpp>
#include <divvy/rpc/tests/KeyGeneration.test.cpp>
#include <divvy/rpc/tests/Status.test.cpp>
#include <divvy/rpc/tests/Yield.test.cpp>
