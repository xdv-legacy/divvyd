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

#ifndef RIPPLE_PROTOCOL_RIPPLEADDRESS_H_INCLUDED
#define RIPPLE_PROTOCOL_RIPPLEADDRESS_H_INCLUDED

#include <divvy/basics/base_uint.h>
#include <divvy/crypto/Base58Data.h>
#include <divvy/crypto/ECDSACanonical.h>
#include <divvy/crypto/KeyType.h>
#include <divvy/json/json_value.h>
#include <divvy/protocol/DivvyPublicKey.h>
#include <divvy/protocol/UInt160.h>
#include <divvy/protocol/UintTypes.h>
#include <beast/utility/noexcept.h>

namespace divvy {

enum VersionEncoding
{
    VER_NONE                = 1,
    VER_NODE_PUBLIC         = 28,
    VER_NODE_PRIVATE        = 32,
    VER_ACCOUNT_ID          = 0,
    VER_ACCOUNT_PUBLIC      = 35,
    VER_ACCOUNT_PRIVATE     = 34,
    VER_FAMILY_GENERATOR    = 41,
    VER_FAMILY_SEED         = 33,
};

//
// Used to hold addresses and parse and produce human formats.
//
// XXX This needs to be reworked to store data in uint160 and uint256.

class DivvyAddress : private CBase58Data
{
private:
    bool    mIsValid;

public:
    DivvyAddress ();

    void const*
    data() const noexcept
    {
        return vchData.data();
    }

    std::size_t
    size() const noexcept
    {
        return vchData.size();
    }

    // For public and private key, checks if they are legal.
    bool isValid () const
    {
        return mIsValid;
    }

    void clear ();
    bool isSet () const;

    static void clearCache ();

    /** Returns the public key.
        Precondition: version == VER_NODE_PUBLIC
    */
    DivvyPublicKey
    toPublicKey() const;

    //
    // Node Public - Also used for Validators
    //
    NodeID getNodeID () const;
    Blob const& getNodePublic () const;

    std::string humanNodePublic () const;

    bool setNodePublic (std::string const& strPublic);
    void setNodePublic (Blob const& vPublic);
    bool verifyNodePublic (uint256 const& hash, Blob const& vchSig,
                           ECDSA mustBeFullyCanonical) const;
    bool verifyNodePublic (uint256 const& hash, std::string const& strSig,
                           ECDSA mustBeFullyCanonical) const;

    static DivvyAddress createNodePublic (DivvyAddress const& naSeed);
    static DivvyAddress createNodePublic (Blob const& vPublic);
    static DivvyAddress createNodePublic (std::string const& strPublic);

    //
    // Node Private
    //
    Blob const& getNodePrivateData () const;
    uint256 getNodePrivate () const;

    std::string humanNodePrivate () const;

    bool setNodePrivate (std::string const& strPrivate);
    void setNodePrivate (Blob const& vPrivate);
    void setNodePrivate (uint256 hash256);
    void signNodePrivate (uint256 const& hash, Blob& vchSig) const;

    static DivvyAddress createNodePrivate (DivvyAddress const& naSeed);

    //
    // Accounts IDs
    //
    AccountID getAccountID () const;

    std::string humanAccountID () const;

    bool setAccountID (
        std::string const& strAccountID,
        Base58::Alphabet const& alphabet = Base58::getDivvyAlphabet());
    void setAccountID (AccountID const& hash160In);

    static DivvyAddress createAccountID (AccountID const& uiAccountID);

    //
    // Accounts Public
    //
    Blob const& getAccountPublic () const;

    std::string humanAccountPublic () const;

    bool setAccountPublic (std::string const& strPublic);
    void setAccountPublic (Blob const& vPublic);
    void setAccountPublic (DivvyAddress const& generator, int seq);

    bool accountPublicVerify (Blob const& message, Blob const& vucSig,
                              ECDSA mustBeFullyCanonical) const;

    static DivvyAddress createAccountPublic (Blob const& vPublic)
    {
        DivvyAddress naNew;
        naNew.setAccountPublic (vPublic);
        return naNew;
    }

    static std::string createHumanAccountPublic (Blob const& vPublic)
    {
        return createAccountPublic (vPublic).humanAccountPublic ();
    }

    // Create a deterministic public key from a public generator.
    static DivvyAddress createAccountPublic (
        DivvyAddress const& naGenerator, int iSeq);

    //
    // Accounts Private
    //
    uint256 getAccountPrivate () const;

    bool setAccountPrivate (std::string const& strPrivate);
    void setAccountPrivate (Blob const& vPrivate);
    void setAccountPrivate (uint256 hash256);
    void setAccountPrivate (DivvyAddress const& naGenerator,
                            DivvyAddress const& naSeed, int seq);

    Blob accountPrivateSign (Blob const& message) const;

    // Encrypt a message.
    Blob accountPrivateEncrypt (
        DivvyAddress const& naPublicTo, Blob const& vucPlainText) const;

    // Decrypt a message.
    Blob accountPrivateDecrypt (
        DivvyAddress const& naPublicFrom, Blob const& vucCipherText) const;

    static DivvyAddress createAccountPrivate (
        DivvyAddress const& generator, DivvyAddress const& seed, int iSeq);

    static DivvyAddress createAccountPrivate (Blob const& vPrivate)
    {
        DivvyAddress   naNew;

        naNew.setAccountPrivate (vPrivate);

        return naNew;
    }

    //
    // Generators
    // Use to generate a master or regular family.
    //
    Blob const& getGenerator () const;

    std::string humanGenerator () const;

    bool setGenerator (std::string const& strGenerator);
    void setGenerator (Blob const& vPublic);
    // void setGenerator(DivvyAddress const& seed);

    // Create generator for making public deterministic keys.
    static DivvyAddress createGeneratorPublic (DivvyAddress const& naSeed);

    //
    // Seeds
    // Clients must disallow reconizable entries from being seeds.
    uint128 getSeed () const;

    std::string humanSeed () const;
    std::string humanSeed1751 () const;

    bool setSeed (std::string const& strSeed);
    int setSeed1751 (std::string const& strHuman1751);
    bool setSeedGeneric (std::string const& strText);
    void setSeed (uint128 hash128);
    void setSeedRandom ();

    static DivvyAddress createSeedRandom ();
    static DivvyAddress createSeedGeneric (std::string const& strText);

    std::string ToString () const
        {return static_cast<CBase58Data const&>(*this).ToString();}

    template <class Hasher>
    friend
    void
    hash_append(Hasher& hasher, DivvyAddress const& value)
    {
        using beast::hash_append;
        hash_append(hasher, static_cast<CBase58Data const&>(value));
    }

    friend
    bool
    operator==(DivvyAddress const& lhs, DivvyAddress const& rhs)
    {
        return static_cast<CBase58Data const&>(lhs) ==
               static_cast<CBase58Data const&>(rhs);
    }

    friend
    bool
    operator <(DivvyAddress const& lhs, DivvyAddress const& rhs)
    {
        return static_cast<CBase58Data const&>(lhs) <
               static_cast<CBase58Data const&>(rhs);
    }
};

//------------------------------------------------------------------------------

inline
bool
operator!=(DivvyAddress const& lhs, DivvyAddress const& rhs)
{
    return !(lhs == rhs);
}

inline
bool
operator >(DivvyAddress const& lhs, DivvyAddress const& rhs)
{
    return rhs < lhs;
}

inline
bool
operator<=(DivvyAddress const& lhs, DivvyAddress const& rhs)
{
    return !(rhs < lhs);
}

inline
bool
operator>=(DivvyAddress const& lhs, DivvyAddress const& rhs)
{
    return !(lhs < rhs);
}

struct KeyPair
{
    DivvyAddress secretKey;
    DivvyAddress publicKey;
};

uint256 keyFromSeed (uint128 const& seed);

DivvyAddress getSeedFromRPC (Json::Value const& params);

KeyPair generateKeysFromSeed (KeyType keyType, DivvyAddress const& seed);

} // divvy

#endif
