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
#include <divvy/core/DatabaseCon.h>
#include <divvy/app/main/Application.h>
#include <divvy/app/main/LocalCredentials.h>
#include <divvy/app/misc/UniqueNodeList.h>
#include <divvy/basics/Log.h>
#include <divvy/basics/StringUtilities.h>
#include <divvy/core/Config.h>
#include <boost/optional.hpp>
#include <iostream>

namespace divvy {

void LocalCredentials::start ()
{
    // We need our node identity before we begin networking.
    // - Allows others to identify if they have connected multiple times.
    // - Determines our CAS routing and responsibilities.
    // - This is not our validation identity.
    if (!nodeIdentityLoad ())
    {
        nodeIdentityCreate ();

        if (!nodeIdentityLoad ())
            throw std::runtime_error ("unable to retrieve new node identity.");
    }

    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: " << mNodePublicKey.humanNodePublic () << std::endl;

    getApp().getUNL ().start ();
}

// Retrieve network identity.
bool LocalCredentials::nodeIdentityLoad ()
{
    auto db = getApp().getWalletDB ().checkoutDb ();
    bool        bSuccess    = false;

    boost::optional<std::string> pubKO, priKO;
    soci::statement st = (db->prepare <<
                          "SELECT PublicKey, PrivateKey "
                          "FROM NodeIdentity;",
                          soci::into(pubKO),
                          soci::into(priKO));
    st.execute ();
    while (st.fetch ())
    {
        mNodePublicKey.setNodePublic (pubKO.value_or(""));
        mNodePrivateKey.setNodePrivate (priKO.value_or(""));

        bSuccess    = true;
    }

    if (getConfig ().NODE_PUB.isValid () && getConfig ().NODE_PRIV.isValid ())
    {
        mNodePublicKey = getConfig ().NODE_PUB;
        mNodePrivateKey = getConfig ().NODE_PRIV;
    }

    return bSuccess;
}

// Create and store a network identity.
bool LocalCredentials::nodeIdentityCreate ()
{
    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: Creating." << std::endl;

    //
    // Generate the public and private key
    //
    DivvyAddress   naSeed          = DivvyAddress::createSeedRandom ();
    DivvyAddress   naNodePublic    = DivvyAddress::createNodePublic (naSeed);
    DivvyAddress   naNodePrivate   = DivvyAddress::createNodePrivate (naSeed);

    //
    // Store the node information
    //
    auto db = getApp().getWalletDB ().checkoutDb ();

    *db << str (boost::format (
        "INSERT INTO NodeIdentity (PublicKey,PrivateKey) VALUES ('%s','%s');")
            % naNodePublic.humanNodePublic ()
            % naNodePrivate.humanNodePrivate ());

    if (!getConfig ().QUIET)
        std::cerr << "NodeIdentity: Created." << std::endl;

    return true;
}

} // divvy
