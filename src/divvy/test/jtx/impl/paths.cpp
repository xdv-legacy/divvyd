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
#include <divvy/test/jtx/paths.h>
#include <divvy/app/paths/FindPaths.h>
#include <divvy/protocol/JsonFields.h>

namespace divvy {
namespace test {
namespace jtx {

void
paths::operator()(Env const& env, JTx& jt) const
{
    auto& jv = jt.jv;
    auto const from = env.lookup(
        jv[jss::Account].asString());
    auto const to = env.lookup(
        jv[jss::Destination].asString());
    auto const amount = amountFromJson(
        sfAmount, jv[jss::Amount]);
    STPath fp;
    STPathSet ps;
    auto const found = findPathsForOneIssuer(
        std::make_shared<DivvyLineCache>(
            env.ledger), from, to,
                in_, amount,
                    depth_, limit_, ps, fp);
    // VFALCO TODO API to allow caller to examine the STPathSet
    // VFALCO isDefault should be renamed to empty()
    if (found && ! ps.isDefault())
        jv[jss::Paths] = ps.getJson(0);
}

} // jtx
} // test
} // divvy
