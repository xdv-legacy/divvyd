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
#include <divvy/basics/TestSuite.h>
#include <divvy/protocol/DivvyAddress.h>
#include <divvy/protocol/DivvyPublicKey.h>
#include <divvy/protocol/Serializer.h>
#include <divvy/basics/StringUtilities.h>

namespace divvy {

class DivvyAddress_test : public divvy::TestSuite
{
public:
    void run()
    {
        // Construct a seed.
        DivvyAddress naSeed;

        expect (naSeed.setSeedGeneric ("masterpassphrase"));
        expect (naSeed.humanSeed () == "snoPBrXtMeMyMHUVTgbuqAfg1SUTb", naSeed.humanSeed ());

        // Create node public/private key pair
        DivvyAddress naNodePublic    = DivvyAddress::createNodePublic (naSeed);
        DivvyAddress naNodePrivate   = DivvyAddress::createNodePrivate (naSeed);

        expect (naNodePublic.humanNodePublic () == "n94a1u4jAz288pZLtw6yFWVbi89YamiC6JBXPVUj5zmExe5fTVg9", naNodePublic.humanNodePublic ());
        expect (naNodePrivate.humanNodePrivate () == "pnen77YEeUd4fFKG7iycBWcwKpTaeFRkW2WFostaATy1DSupwXe", naNodePrivate.humanNodePrivate ());

        // Check node signing.
        Blob vucTextSrc = strCopy ("Hello, nurse!");
        uint256 uHash   = sha512Half(make_Slice(vucTextSrc));
        Blob vucTextSig;

        naNodePrivate.signNodePrivate (uHash, vucTextSig);
        expect (naNodePublic.verifyNodePublic (uHash, vucTextSig, ECDSA::strict), "Verify failed.");

        // Construct a public generator from the seed.
        DivvyAddress   generator     = DivvyAddress::createGeneratorPublic (naSeed);

        expect (generator.humanGenerator () == "fhuJKrhSDzV2SkjLn9qbwm5AaRmrxDPfFsHDCP6yfDZWcxDFz4mt", generator.humanGenerator ());

        // Create ed25519 account public/private key pair.
        KeyPair keys = generateKeysFromSeed (KeyType::ed25519, naSeed);
        expectEquals (keys.publicKey.humanAccountPublic(), "aKGheSBjmCsKJVuLNKRAKpZXT6wpk2FCuEZAXJupXgdAxX5THCqR");

        // Check ed25519 account signing.
        vucTextSig = keys.secretKey.accountPrivateSign (vucTextSrc);

        expect (!vucTextSig.empty(), "ed25519 signing failed.");
        expect (keys.publicKey.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA()), "ed25519 verify failed.");

        // Create account #0 public/private key pair.
        DivvyAddress   naAccountPublic0    = DivvyAddress::createAccountPublic (generator, 0);
        DivvyAddress   naAccountPrivate0   = DivvyAddress::createAccountPrivate (generator, naSeed, 0);

        expect (naAccountPublic0.humanAccountID () == "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", naAccountPublic0.humanAccountID ());
        expect (naAccountPublic0.humanAccountPublic () == "aBQG8RQAzjs1eTKFEAQXr2gS4utcDiEC9wmi7pfUPTi27VCahwgw", naAccountPublic0.humanAccountPublic ());

        // Create account #1 public/private key pair.
        DivvyAddress   naAccountPublic1    = DivvyAddress::createAccountPublic (generator, 1);
        DivvyAddress   naAccountPrivate1   = DivvyAddress::createAccountPrivate (generator, naSeed, 1);

        expect (naAccountPublic1.humanAccountID () == "r4bYF7SLUMD7QgSLLpgJx38WJSY12ViRjP", naAccountPublic1.humanAccountID ());
        expect (naAccountPublic1.humanAccountPublic () == "aBPXpTfuLy1Bhk3HnGTTAqnovpKWQ23NpFMNkAF6F1Atg5vDyPrw", naAccountPublic1.humanAccountPublic ());

        // Check account signing.
        vucTextSig = naAccountPrivate0.accountPrivateSign (vucTextSrc);

        expect (!vucTextSig.empty(), "Signing failed.");
        expect (naAccountPublic0.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::strict), "Verify failed.");
        expect (!naAccountPublic1.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::not_strict), "Anti-verify failed.");
        expect (!naAccountPublic1.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::strict), "Anti-verify failed.");

        vucTextSig = naAccountPrivate1.accountPrivateSign (vucTextSrc);

        expect (!vucTextSig.empty(), "Signing failed.");
        expect (naAccountPublic1.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::strict), "Verify failed.");
        expect (!naAccountPublic0.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::not_strict), "Anti-verify failed.");
        expect (!naAccountPublic0.accountPublicVerify (vucTextSrc, vucTextSig, ECDSA::strict), "Anti-verify failed.");

        // Check account encryption.
        Blob vucTextCipher
            = naAccountPrivate0.accountPrivateEncrypt (naAccountPublic1, vucTextSrc);
        Blob vucTextRecovered
            = naAccountPrivate1.accountPrivateDecrypt (naAccountPublic0, vucTextCipher);

        expect (vucTextSrc == vucTextRecovered, "Encrypt-decrypt failed.");

        {
            DivvyAddress nSeed;
            uint128 seed1, seed2;
            seed1.SetHex ("71ED064155FFADFA38782C5E0158CB26");
            nSeed.setSeed (seed1);
            expect (nSeed.humanSeed() == "shHM53KPZ87Gwdqarm1bAmPeXg8Tn",
                "Incorrect human seed");
            expect (nSeed.humanSeed1751() == "MAD BODY ACE MINT OKAY HUB WHAT DATA SACK FLAT DANA MATH",
                "Incorrect 1751 seed");
        }
    }
};

//------------------------------------------------------------------------------

class DivvyIdentifier_test : public beast::unit_test::suite
{
public:
    void run ()
    {
        testcase ("Seed");
        DivvyAddress seed;
        expect (seed.setSeedGeneric ("masterpassphrase"));
        expect (seed.humanSeed () == "snoPBrXtMeMyMHUVTgbuqAfg1SUTb", seed.humanSeed ());

        testcase ("DivvyPublicKey");
        DivvyAddress deprecatedPublicKey (DivvyAddress::createNodePublic (seed));
        expect (deprecatedPublicKey.humanNodePublic () ==
            "n94a1u4jAz288pZLtw6yFWVbi89YamiC6JBXPVUj5zmExe5fTVg9",
                deprecatedPublicKey.humanNodePublic ());
        DivvyPublicKey publicKey = deprecatedPublicKey.toPublicKey();
        expect (publicKey.to_string() == deprecatedPublicKey.humanNodePublic(),
            publicKey.to_string());

        testcase ("Generator");
        DivvyAddress generator (DivvyAddress::createGeneratorPublic (seed));
        expect (generator.humanGenerator () ==
            "fhuJKrhSDzV2SkjLn9qbwm5AaRmrxDPfFsHDCP6yfDZWcxDFz4mt",
                generator.humanGenerator ());
    }
};

BEAST_DEFINE_TESTSUITE(DivvyAddress,divvy_data,divvy);
BEAST_DEFINE_TESTSUITE(DivvyIdentifier,divvy_data,divvy);

} // divvy
