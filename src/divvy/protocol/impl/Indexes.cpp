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
#include <divvy/basics/SHA512Half.h>
#include <divvy/protocol/Indexes.h>
#include <beast/utility/static_initializer.h>
#include <cassert>

namespace divvy {

// get the index of the node that holds the last 256 ledgers
uint256
getLedgerHashIndex ()
{
    return sha512Half(std::uint16_t(spaceSkipList));
}

// Get the index of the node that holds the set of 256 ledgers that includes
// this ledger's hash (or the first ledger after it if it's not a multiple
// of 256).
uint256
getLedgerHashIndex (std::uint32_t desiredLedgerIndex)
{
    return sha512Half(
        std::uint16_t(spaceSkipList),
        std::uint32_t(desiredLedgerIndex >> 16));
}

// get the index of the node that holds the enabled amendments
uint256
getLedgerAmendmentIndex ()
{
    return sha512Half(std::uint16_t(spaceAmendment));
}

// get the index of the node that holds the fee schedule
uint256
getLedgerFeeIndex ()
{
    return sha512Half(std::uint16_t(spaceFee));
}

uint256
getAccountRootIndex (AccountID const& account)
{
    return sha512Half(
        std::uint16_t(spaceAccount),
        account);
}

uint256
getAccountRootIndex (const DivvyAddress & account)
{
    return getAccountRootIndex (account.getAccountID ());
}

uint256
getGeneratorIndex (AccountID const& uGeneratorID)
{
    return sha512Half(
        std::uint16_t(spaceGenerator),
        uGeneratorID);
}

uint256
getBookBase (Book const& book)
{
    assert (isConsistent (book));
    // Return with quality 0.
    return getQualityIndex(sha512Half(
        std::uint16_t(spaceBookDir),
        book.in.currency,
        book.out.currency,
        book.in.account,
        book.out.account));
}

uint256
getOfferIndex (AccountID const& account, std::uint32_t uSequence)
{
    return sha512Half(
        std::uint16_t(spaceOffer),
        account,
        std::uint32_t(uSequence));
}

uint256
getOwnerDirIndex (AccountID const& account)
{
    return sha512Half(
        std::uint16_t(spaceOwnerDir),
        account);
}


uint256
getDirNodeIndex (uint256 const& uDirRoot, const std::uint64_t uNodeIndex)
{
    if (uNodeIndex == 0)
        return uDirRoot;

    return sha512Half(
        std::uint16_t(spaceDirNode),
        uDirRoot,
        std::uint64_t(uNodeIndex));
}

uint256
getQualityIndex (uint256 const& uBase, const std::uint64_t uNodeDir)
{
    // Indexes are stored in big endian format: they print as hex as stored.
    // Most significant bytes are first.  Least significant bytes represent
    // adjacent entries.  We place uNodeDir in the 8 right most bytes to be
    // adjacent.  Want uNodeDir in big endian format so ++ goes to the next
    // entry for indexes.
    uint256 uNode (uBase);

    // TODO(tom): there must be a better way.
    // VFALCO [base_uint] This assumes a certain storage format
    ((std::uint64_t*) uNode.end ())[-1] = htobe64 (uNodeDir);

    return uNode;
}

uint256
getQualityNext (uint256 const& uBase)
{
    static beast::static_initializer<uint256> const uNext (
        from_hex_text<uint256>("10000000000000000"));
    return uBase + *uNext;
}

std::uint64_t
getQuality (uint256 const& uBase)
{
    // VFALCO [base_uint] This assumes a certain storage format
    return be64toh (((std::uint64_t*) uBase.end ())[-1]);
}

uint256
getTicketIndex (AccountID const& account, std::uint32_t uSequence)
{
    return sha512Half(
        std::uint16_t(spaceTicket),
        account,
        std::uint32_t(uSequence));
}

uint256
getDivvyStateIndex (AccountID const& a, AccountID const& b, Currency const& currency)
{
    if (a < b)
        return sha512Half(
            std::uint16_t(spaceDivvy),
            a,
            b,
            currency);
    return sha512Half(
        std::uint16_t(spaceDivvy),
        b,
        a,
        currency);
}

uint256
getDivvyStateIndex (AccountID const& a, Issue const& issue)
{
    return getDivvyStateIndex (a, issue.account, issue.currency);
}

uint256
getSignerListIndex (AccountID const& account)
{
    return sha512Half(
        std::uint16_t(spaceSignerList),
        account);
}

//------------------------------------------------------------------------------

namespace keylet {

Keylet account_t::operator()(
    AccountID const& id) const
{
    return { ltACCOUNT_ROOT,
        getAccountRootIndex(id) };
}

Keylet account_t::operator()(
    DivvyAddress const& ra) const
{
    return { ltACCOUNT_ROOT,
        getAccountRootIndex(ra.getAccountID()) };
}

Keylet owndir_t::operator()(
    AccountID const& id) const
{
    return { ltDIR_NODE,
        getOwnerDirIndex(id) };
}

Keylet skip_t::operator()() const
{
    return { ltLEDGER_HASHES,
        getLedgerHashIndex() };
}

Keylet skip_t::operator()(LedgerIndex ledger) const
{
    return { ltLEDGER_HASHES,
        getLedgerHashIndex(ledger) };
}

Keylet amendments_t::operator()() const
{
    return { ltAMENDMENTS,
        getLedgerAmendmentIndex() };
}

Keylet fee_t::operator()() const
{
    return { ltFEE_SETTINGS,
        getLedgerFeeIndex() };
}

Keylet book_t::operator()(Book const& b) const
{
    return { ltDIR_NODE,
        getBookBase(b) };
}

Keylet offer_t::operator()(AccountID const& id,
    std::uint32_t seq) const
{
    return { ltOFFER,
        getOfferIndex(id, seq) };
}

Keylet item_t::operator()(Keylet const& k,
    std::uint64_t index,
        LedgerEntryType type) const
{
    return { type,
        getDirNodeIndex(k.key, index) };
}

Keylet quality_t::operator()(Keylet const& k,
    std::uint64_t q) const
{
    assert(k.type == ltDIR_NODE);
    return { ltDIR_NODE,
        getQualityIndex(k.key, q) };
}

Keylet next_t::operator()(Keylet const& k) const
{
    assert(k.type == ltDIR_NODE);
    return { ltDIR_NODE,
        getQualityNext(k.key) };
}

Keylet ticket_t::operator()(AccountID const& id,
    std::uint32_t seq) const
{
    return { ltTICKET,
        getTicketIndex(id, seq) };
}

Keylet trust_t::operator()(AccountID const& id0,
    AccountID const& id1, Currency const& currency) const
{
    return { ltRIPPLE_STATE,
        getDivvyStateIndex(id0, id1, currency) };
}

Keylet trust_t::operator()(AccountID const& id,
    Issue const& issue) const
{
    return { ltRIPPLE_STATE,
        getDivvyStateIndex(id, issue) };
}

Keylet signers_t::operator()(AccountID const& id) const
{
    return { ltSIGNER_LIST,
        getSignerListIndex(id) };
}

} // keylet

} // divvy
