//------------------------------------------------------------------------------
/*
  This file is part of divvyd: https://github.com/xdv/divvyd
  Copyright (c) 2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED
#define RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED

#include <divvy/protocol/DivvyAddress.h>
#include <divvy/protocol/UintTypes.h>
#include <divvy/crypto/KeyType.h>
#include <beast/utility/noexcept.h>
#include <string>

namespace divvy {
namespace test {
namespace jtx {

class IOU;

/** Immutable cryptographic account descriptor. */
class Account
{
private:
    std::string name_;
    // VFALCO TODO use AnyPublicKey, AnySecretKey
    //             instead of DivvyAddress
    DivvyAddress pk_;
    DivvyAddress sk_;
    AccountID id_;
    std::string human_; // base58 public key string

public:
    Account() = default;
    Account (Account const&) = default;
    Account& operator= (Account const&) = default;

#ifdef _MSC_VER
    Account (Account&&);
    Account& operator= (Account&&);
#else
    Account (Account&&) = default;
    Account& operator= (Account&&) = default;
#endif

    /** Create an account from a key pair. */
    Account (std::string name, KeyPair&& keys);

    /** Create an account from a simple string name. */
    /** @{ */
    Account (std::string name,
        KeyType type = KeyType::secp256k1);
    Account (char const* name,
        KeyType type = KeyType::secp256k1)
        : Account(std::string(name), type)
    {
    }
    /** @} */

    /** Return the name */
    std::string const&
    name() const
    {
        return name_;
    }

    /** Return the public key. */
    DivvyAddress const&
    pk() const
    {
        return pk_;
    }

    /** Return the secret key. */
    DivvyAddress const&
    sk() const
    {
        return sk_;
    }

    /** Returns the Account ID.

        The Account ID is the uint160 hash of the public key.
    */
    AccountID
    id() const
    {
        return id_;
    }

    /** Returns the human readable public key. */
    std::string const&
    human() const
    {
        return human_;
    }

    /** Implicit conversion to AccountID.

        This allows passing an Account
        where an AccountID is expected.
    */
    operator AccountID() const
    {
        return id_;
    }

    /** Returns an IOU for the specified gateway currency. */
    IOU
    operator[](std::string const& s) const;
};

inline
bool
operator== (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() == rhs.id();
}

template <class Hasher>
void
hash_append (Hasher& h,
    Account const& v) noexcept
{
    hash_append(h, v.id());
}

inline
bool
operator< (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() < rhs.id();
}

} // jtx
} // test
} // divvy

#endif
