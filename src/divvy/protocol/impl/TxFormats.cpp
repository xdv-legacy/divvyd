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
#include <beast/utility/static_initializer.h>
#include <divvy/protocol/TxFormats.h>

namespace divvy {

TxFormats::TxFormats ()
{
    add ("AccountSet", ttACCOUNT_SET)
        << SOElement (sfEmailHash,           SOE_OPTIONAL)
        << SOElement (sfWalletLocator,       SOE_OPTIONAL)
        << SOElement (sfWalletSize,          SOE_OPTIONAL)
        << SOElement (sfMessageKey,          SOE_OPTIONAL)
        << SOElement (sfDomain,              SOE_OPTIONAL)
        << SOElement (sfTransferRate,        SOE_OPTIONAL)
        << SOElement (sfSetFlag,             SOE_OPTIONAL)
        << SOElement (sfClearFlag,           SOE_OPTIONAL)
        ;

    add ("TrustSet", ttTRUST_SET)
        << SOElement (sfLimitAmount,         SOE_OPTIONAL)
        << SOElement (sfQualityIn,           SOE_OPTIONAL)
        << SOElement (sfQualityOut,          SOE_OPTIONAL)
        ;

    add ("OfferCreate", ttOFFER_CREATE)
        << SOElement (sfTakerPays,           SOE_REQUIRED)
        << SOElement (sfTakerGets,           SOE_REQUIRED)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        << SOElement (sfOfferSequence,       SOE_OPTIONAL)
        ;

    add ("OfferCancel", ttOFFER_CANCEL)
        << SOElement (sfOfferSequence,       SOE_REQUIRED)
        ;

    add ("SetRegularKey", ttREGULAR_KEY_SET)
        << SOElement (sfRegularKey,          SOE_OPTIONAL)
        ;

    add ("Payment", ttPAYMENT)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfSendMax,             SOE_OPTIONAL)
        << SOElement (sfPaths,               SOE_DEFAULT)
        << SOElement (sfInvoiceID,           SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        ;

    add ("EnableAmendment", ttAMENDMENT)
        << SOElement (sfLedgerSequence,      SOE_OPTIONAL)
        << SOElement (sfAmendment,           SOE_REQUIRED)
        ;

    add ("SetFee", ttFEE)
        << SOElement (sfLedgerSequence,      SOE_OPTIONAL)
        << SOElement (sfBaseFee,             SOE_REQUIRED)
        << SOElement (sfReferenceFeeUnits,   SOE_REQUIRED)
        << SOElement (sfReserveBase,         SOE_REQUIRED)
        << SOElement (sfReserveIncrement,    SOE_REQUIRED)
        ;

    add ("TicketCreate", ttTICKET_CREATE)
        << SOElement (sfTarget,              SOE_OPTIONAL)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        ;

    add ("TicketCancel", ttTICKET_CANCEL)
        << SOElement (sfTicketID,            SOE_REQUIRED)
        ;

    // The SignerEntries are optional because a SignerList is deleted by
    // setting the SignerQuorum to zero and omitting SignerEntries.
    add ("SignerListSet", ttSIGNER_LIST_SET)
        << SOElement (sfSignerQuorum,        SOE_REQUIRED)
        << SOElement (sfSignerEntries,       SOE_OPTIONAL)
        ;
}

void TxFormats::addCommonFields (Item& item)
{
    item
        << SOElement(sfTransactionType,     SOE_REQUIRED)
        << SOElement(sfFlags,               SOE_OPTIONAL)
        << SOElement(sfSourceTag,           SOE_OPTIONAL)
        << SOElement(sfAccount,             SOE_REQUIRED)
        << SOElement(sfSequence,            SOE_REQUIRED)
        << SOElement(sfPreviousTxnID,       SOE_OPTIONAL) // emulate027
        << SOElement(sfLastLedgerSequence,  SOE_OPTIONAL)
        << SOElement(sfAccountTxnID,        SOE_OPTIONAL)
        << SOElement(sfFee,                 SOE_REQUIRED)
        << SOElement(sfOperationLimit,      SOE_OPTIONAL)
        << SOElement(sfMemos,               SOE_OPTIONAL)
        << SOElement(sfSigningPubKey,       SOE_REQUIRED)
        << SOElement(sfTxnSignature,        SOE_OPTIONAL)
        << SOElement(sfMultiSigners,        SOE_OPTIONAL) // submit_multisigned
        ;
}

TxFormats const&
TxFormats::getInstance ()
{
    static beast::static_initializer<
        TxFormats> instance;
    return *instance;
}

} // divvy
