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
#include <divvy/protocol/Serializer.h>
#include <divvy/protocol/SystemParameters.h>
#include <divvy/protocol/DivvyAddress.h>
#include <divvy/protocol/UintTypes.h>
#include <beast/utility/static_initializer.h>

namespace divvy {

std::string to_string(AccountID const& account)
{
    return DivvyAddress::createAccountID (account).humanAccountID ();
}

std::string to_string(Currency const& currency)
{
    // Characters we are willing to allow in the ASCII representation of a
    // three-letter currency code.
    static std::string const allowed_characters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "<>(){}[]|?!@#$%^&*";

    if (currency == zero)
        return systemCurrencyCode();

    if (currency == noCurrency())
        return "1";

    static beast::static_initializer<Currency> const sIsoBits (
        from_hex_text<Currency>("FFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFF"));

    if ((currency & *sIsoBits).isZero ())
    {
        // The offset of the 3 character ISO code in the currency descriptor
        int const isoOffset = 12;

        std::string const iso(
            currency.data () + isoOffset,
            currency.data () + isoOffset + 3);

        // Specifying the system currency code using ISO-style representation
        // is not allowed.
        if ((iso != systemCurrencyCode()) &&
            (iso.find_first_not_of (allowed_characters) == std::string::npos))
        {
            return iso;
        }
    }

    return strHex (currency.begin (), currency.size ());
}

bool to_currency(Currency& currency, std::string const& code)
{
    if (code.empty () || !code.compare (systemCurrencyCode()))
    {
        currency = zero;
        return true;
    }

    static const int CURRENCY_CODE_LENGTH = 3;
    if (code.size () == CURRENCY_CODE_LENGTH)
    {
        Blob codeBlob (CURRENCY_CODE_LENGTH);

        std::transform (code.begin (), code.end (), codeBlob.begin (), ::toupper);

        Serializer  s;

        s.addZeros (96 / 8);
        s.addRaw (codeBlob);
        s.addZeros (16 / 8);
        s.addZeros (24 / 8);

        s.get160 (currency, 0);
        return true;
    }

    if (40 == code.size ())
        return currency.SetHex (code);

    return false;
}

Currency to_currency(std::string const& code)
{
    Currency currency;
    if (!to_currency(currency, code))
        currency = noCurrency();
    return currency;
}

bool to_issuer(AccountID& issuer, std::string const& s)
{
    if (s.size () == (160 / 4))
    {
        issuer.SetHex (s);
        return true;
    }
    DivvyAddress address;
    bool success = address.setAccountID (s);
    if (success)
        issuer = address.getAccountID ();
    return success;
}

AccountID const& xdvAccount()
{
    static AccountID const account(0);
    return account;
}

Currency const& xdvCurrency()
{
    static Currency const currency(0);
    return currency;
}

AccountID const& noAccount()
{
    static AccountID const account(1);
    return account;
}

Currency const& noCurrency()
{
    static Currency const currency(1);
    return currency;
}

Currency const& badCurrency()
{
    static Currency const currency(0x5852500000000000);
    return currency;
}

} // divvy
