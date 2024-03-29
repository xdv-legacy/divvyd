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

#ifndef RIPPLE_PROTOCOL_STTX_H_INCLUDED
#define RIPPLE_PROTOCOL_STTX_H_INCLUDED

#include <divvy/protocol/STObject.h>
#include <divvy/protocol/TxFormats.h>
#include <boost/logic/tribool.hpp>

namespace divvy {

// VFALCO TODO replace these macros with language constants
#define TXN_SQL_NEW         'N'
#define TXN_SQL_CONFLICT    'C'
#define TXN_SQL_HELD        'H'
#define TXN_SQL_VALIDATED   'V'
#define TXN_SQL_INCLUDED    'I'
#define TXN_SQL_UNKNOWN     'U'

class STTx final
    : public STObject
    , public CountedObject <STTx>
{
public:
    static char const* getCountedObjectName () { return "STTx"; }

    using pointer = std::shared_ptr<STTx>;
    using ref     = const std::shared_ptr<STTx>&;

    static std::size_t const maxMultiSigners = 8;

public:
    STTx () = delete;
    STTx& operator= (STTx const& other) = delete;

    STTx (STTx const& other) = default;

    explicit STTx (SerialIter& sit);
    explicit STTx (TxType type);

    explicit STTx (STObject&& object);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    // STObject functions.
    SerializedTypeID getSType () const override
    {
        return STI_TRANSACTION;
    }
    std::string getFullText () const override;

    // Outer transaction functions / signature functions.
    Blob getSignature () const;

    uint256 getSigningHash () const;

    TxType getTxnType () const
    {
        return tx_type_;
    }
    STAmount getTransactionFee () const
    {
        return getFieldAmount (sfFee);
    }
    void setTransactionFee (const STAmount & fee)
    {
        setFieldAmount (sfFee, fee);
    }

    DivvyAddress getSourceAccount () const
    {
        return getFieldAccount (sfAccount);
    }
    Blob getSigningPubKey () const
    {
        return getFieldVL (sfSigningPubKey);
    }
    void setSigningPubKey (const DivvyAddress & naSignPubKey);
    void setSourceAccount (const DivvyAddress & naSource);

    std::uint32_t getSequence () const
    {
        return getFieldU32 (sfSequence);
    }
    void setSequence (std::uint32_t seq)
    {
        return setFieldU32 (sfSequence, seq);
    }

    std::vector<DivvyAddress> getMentionedAccounts () const;

    uint256 getTransactionID () const;

    Json::Value getJson (int options) const override;
    Json::Value getJson (int options, bool binary) const;

    void sign (DivvyAddress const& private_key);

    bool checkSign(bool allowMultiSign =
#if RIPPLE_ENABLE_MULTI_SIGN
        true
#else
        false
#endif
            ) const;

    bool isKnownGood () const
    {
        return (sig_state_ == true);
    }
    bool isKnownBad () const
    {
        return (sig_state_ == false);
    }
    void setGood () const
    {
        sig_state_ = true;
    }
    void setBad () const
    {
        sig_state_ = false;
    }

    // SQL Functions with metadata.
    static
    std::string const&
    getMetaSQLInsertReplaceHeader ();

    std::string getMetaSQL (
        std::uint32_t inLedger, std::string const& escapedMetaData) const;

    std::string getMetaSQL (
        Serializer rawTxn,
        std::uint32_t inLedger,
        char status,
        std::string const& escapedMetaData) const;

private:
    bool checkSingleSign () const;
    bool checkMultiSign () const;

    TxType tx_type_;

    mutable boost::tribool sig_state_;
};

bool passesLocalChecks (STObject const& st, std::string&);

} // divvy

#endif
