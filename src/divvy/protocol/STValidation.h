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

#ifndef RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED
#define RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED

#include <divvy/protocol/DivvyAddress.h>
#include <divvy/protocol/STObject.h>
#include <cstdint>
#include <memory>

namespace divvy {

// Validation flags
const std::uint32_t vfFullyCanonicalSig    = 0x80000000; // signature is fully canonical

class STValidation final
    : public STObject
    , public CountedObject <STValidation>
{
public:
    static char const* getCountedObjectName () { return "STValidation"; }

    using pointer = std::shared_ptr<STValidation>;
    using ref     = const std::shared_ptr<STValidation>&;

    enum
    {
        kFullFlag = 0x1
    };

    // These throw if the object is not valid
    STValidation (SerialIter & sit, bool checkSignature = true);

    // Does not sign the validation
    STValidation (uint256 const& ledgerHash, std::uint32_t signTime,
                          const DivvyAddress & raPub, bool isFull);

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

    uint256         getLedgerHash ()     const;
    std::uint32_t   getSignTime ()       const;
    std::uint32_t   getFlags ()          const;
    DivvyAddress   getSignerPublic ()   const;
    NodeID          getNodeID ()         const
    {
        return mNodeID;
    }
    bool            isValid ()           const;
    bool            isFull ()            const;
    bool            isTrusted ()         const
    {
        return mTrusted;
    }
    uint256         getSigningHash ()    const;
    bool            isValid (uint256 const& ) const;

    void                        setTrusted ()
    {
        mTrusted = true;
    }
    Blob    getSigned ()                 const;
    Blob    getSignature ()              const;

    // Signs the validation and returns the signing hash
    uint256 sign (const DivvyAddress & raPrivate);

    // The validation this replaced
    uint256 const& getPreviousHash ()
    {
        return mPreviousHash;
    }
    bool isPreviousHash (uint256 const& h) const
    {
        return mPreviousHash == h;
    }
    void setPreviousHash (uint256 const& h)
    {
        mPreviousHash = h;
    }

private:
    static SOTemplate const& getFormat ();

    void setNode ();

    uint256 mPreviousHash;
    NodeID mNodeID;
    bool mTrusted;
};

} // divvy

#endif
