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
#include <divvy/protocol/Quality.h>
#include <divvy/app/paths/cursor/DivvyLiquidity.h>
#include <divvy/basics/Log.h>

namespace divvy {
namespace path {

// The reverse pass has been narrowing by credit available and inflating by fees
// as it worked backwards.  Now, for the current account node, take the actual
// amount from previous and adjust forward balances.
//
// Perform balance adjustments between previous and current node.
// - The previous node: specifies what to push through to current.
// - All of previous output is consumed.
//
// Then, compute current node's output for next node.
// - Current node: specify what to push through to next.
// - Output to next node is computed as input minus quality or transfer fee.
// - If next node is an offer and output is non-XDV then we are the issuer and
//   do not need to push funds.
// - If next node is an offer and output is XDV then we need to deliver funds to
//   limbo.
TER PathCursor::forwardLiquidityForAccount () const
{
    TER resultCode   = tesSUCCESS;
    auto const lastNodeIndex       = pathState_.nodes().size () - 1;

    std::uint64_t uRateMax = 0;

    AccountID const& previousAccountID =
            previousNode().isAccount() ? previousNode().account_ :
            node().account_;
    // Offers are always issue.
    AccountID const& nextAccountID =
            nextNode().isAccount() ? nextNode().account_ : node().account_;

    std::uint32_t uQualityIn = nodeIndex_
        ? quality_in (ledger(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : QUALITY_ONE;
    std::uint32_t  uQualityOut = (nodeIndex_ == lastNodeIndex)
        ? quality_out (ledger(),
            node().account_,
            nextAccountID,
            node().issue_.currency)
        : QUALITY_ONE;

    // When looking backward (prv) for req we care about what we just
    // calculated: use fwd.
    // When looking forward (cur) for req we care about what was desired: use
    // rev.

    // For nextNode().isAccount()
    auto saPrvRedeemAct = previousNode().saFwdRedeem.zeroed();
    auto saPrvIssueAct = previousNode().saFwdIssue.zeroed();

    // For !previousNode().isAccount()
    auto saPrvDeliverAct = previousNode().saFwdDeliver.zeroed ();

    WriteLog (lsTRACE, DivvyCalc)
        << "forwardLiquidityForAccount> "
        << "nodeIndex_=" << nodeIndex_ << "/" << lastNodeIndex
        << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
        << " saPrvIssueReq:" << previousNode().saFwdIssue
        << " previousNode.saFwdDeliver:" << previousNode().saFwdDeliver
        << " node.saRevRedeem:" << node().saRevRedeem
        << " node.saRevIssue:" << node().saRevIssue
        << " node.saRevDeliver:" << node().saRevDeliver;

    // Divvy through account.

    if (previousNode().isAccount() && nextNode().isAccount())
    {
        // Next is an account, must be rippling.

        if (!nodeIndex_)
        {
            // ^ --> ACCOUNT --> account

            // For the first node, calculate amount to divvy based on what is
            // available.
            node().saFwdRedeem = node().saRevRedeem;

            if (pathState_.inReq() >= zero)
            {
                // Limit by send max.
                node().saFwdRedeem = std::min (
                    node().saFwdRedeem, pathState_.inReq() - pathState_.inAct());
            }

            pathState_.setInPass (node().saFwdRedeem);

            node().saFwdIssue = node().saFwdRedeem == node().saRevRedeem
                // Fully redeemed.
                ? node().saRevIssue : STAmount (node().saRevIssue);

            if (node().saFwdIssue && pathState_.inReq() >= zero)
            {
                // Limit by send max.
                node().saFwdIssue = std::min (
                    node().saFwdIssue,
                    pathState_.inReq() - pathState_.inAct() - node().saFwdRedeem);
            }

            pathState_.setInPass (pathState_.inPass() + node().saFwdIssue);

            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: ^ --> "
                << "ACCOUNT --> account :"
                << " saInReq=" << pathState_.inReq()
                << " saInAct=" << pathState_.inAct()
                << " node.saFwdRedeem:" << node().saFwdRedeem
                << " node.saRevIssue:" << node().saRevIssue
                << " node.saFwdIssue:" << node().saFwdIssue
                << " pathState_.saInPass:" << pathState_.inPass();
        }
        else if (nodeIndex_ == lastNodeIndex)
        {
            // account --> ACCOUNT --> $
            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> $ :"
                << " previousAccountID="
                << to_string (previousAccountID)
                << " node.account_="
                << to_string (node().account_)
                << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
                << " previousNode.saFwdIssue:" << previousNode().saFwdIssue;

            // Last node. Accept all funds. Calculate amount actually to credit.

            auto& saCurReceive = pathState_.outPass();
            STAmount saIssueCrd = uQualityIn >= QUALITY_ONE
                    ? previousNode().saFwdIssue  // No fee.
                    : mulRound (
                          previousNode().saFwdIssue,
                          amountFromRate (uQualityIn),
                          previousNode().saFwdIssue.issue (),
                          true); // Amount to credit.

            // Amount to credit. Credit for less than received as a surcharge.
            pathState_.setOutPass (previousNode().saFwdRedeem + saIssueCrd);

            if (saCurReceive)
            {
                // Actually receive.
                resultCode = ledger().divvyCredit (
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false);
            }
            else
            {
                // After applying quality, total payment was microscopic.
                resultCode   = tecPATH_DRY;
            }
        }
        else
        {
            // account --> ACCOUNT --> account
            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

            // Previous redeem part 1: redeem -> redeem
            if (previousNode().saFwdRedeem && node().saRevRedeem)
                // Previous wants to redeem.
            {
                // Rate : 1.0 : quality out
                divvyLiquidity (
                    divvyCalc_,
                    QUALITY_ONE,
                    uQualityOut,
                    previousNode().saFwdRedeem,
                    node().saRevRedeem,
                    saPrvRedeemAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            // Previous issue part 1: issue -> redeem
            if (previousNode().saFwdIssue != saPrvIssueAct
                // Previous wants to issue.
                && node().saRevRedeem != node().saFwdRedeem)
                // Current has more to redeem to next.
            {
                // Rate: quality in : quality out
                divvyLiquidity (
                    divvyCalc_,
                    uQualityIn,
                    uQualityOut,
                    previousNode().saFwdIssue,
                    node().saRevRedeem,
                    saPrvIssueAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            // Previous redeem part 2: redeem -> issue.
            if (previousNode().saFwdRedeem != saPrvRedeemAct
                // Previous still wants to redeem.
                && node().saRevRedeem == node().saFwdRedeem
                // Current redeeming is done can issue.
                && node().saRevIssue)
                // Current wants to issue.
            {
                // Rate : 1.0 : transfer_rate
                divvyLiquidity (
                    divvyCalc_,
                    QUALITY_ONE,
                    divvyTransferRate (ledger(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevIssue,
                    saPrvRedeemAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            // Previous issue part 2 : issue -> issue
            if (previousNode().saFwdIssue != saPrvIssueAct
                // Previous wants to issue.
                && node().saRevRedeem == node().saFwdRedeem
                // Current redeeming is done can issue.
                && node().saRevIssue)
                // Current wants to issue.
            {
                // Rate: quality in : 1.0
                divvyLiquidity (
                    divvyCalc_,
                    uQualityIn,
                    QUALITY_ONE,
                    previousNode().saFwdIssue,
                    node().saRevIssue,
                    saPrvIssueAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

            // Adjust prv --> cur balance : take all inbound
            resultCode = saProvide
                ? ledger().divvyCredit (
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false)
                : tecPATH_DRY;
        }
    }
    else if (previousNode().isAccount() && !nextNode().isAccount())
    {
        // Current account is issuer to next offer.
        // Determine deliver to offer amount.
        // Don't adjust outbound balances- keep funds with issuer as limbo.
        // If issuer hold's an offer owners inbound IOUs, there is no fee and
        // redeem/issue will transparently happen.

        if (nodeIndex_)
        {
            // Non-XDV, current node is the issuer.
            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> offer";

            node().saFwdDeliver.clear (node().saRevDeliver);

            // redeem -> issue/deliver.
            // Previous wants to redeem.
            // Current is issuing to an offer so leave funds in account as
            // "limbo".
            if (previousNode().saFwdRedeem)
                // Previous wants to redeem.
            {
                // Rate : 1.0 : transfer_rate
                // XXX Is having the transfer rate here correct?
                divvyLiquidity (
                    divvyCalc_,
                    QUALITY_ONE,
                    divvyTransferRate (ledger(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevDeliver,
                    saPrvRedeemAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

            // issue -> issue/deliver
            if (previousNode().saFwdRedeem == saPrvRedeemAct
                // Previous done redeeming: Previous has no IOUs.
                && previousNode().saFwdIssue)
                // Previous wants to issue. To next must be ok.
            {
                // Rate: quality in : 1.0
                divvyLiquidity (
                    divvyCalc_,
                    uQualityIn,
                    QUALITY_ONE,
                    previousNode().saFwdIssue,
                    node().saRevDeliver,
                    saPrvIssueAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

            // Adjust prv --> cur balance : take all inbound
            resultCode   = node().saFwdDeliver
                ? ledger().divvyCredit (
                    previousAccountID, node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false)
                : tecPATH_DRY;  // Didn't actually deliver anything.
        }
        else
        {
            // Delivering amount requested from downstream.
            node().saFwdDeliver = node().saRevDeliver;

            // If limited, then limit by send max and available.
            if (pathState_.inReq() >= zero)
            {
                // Limit by send max.
                node().saFwdDeliver = std::min (
                    node().saFwdDeliver, pathState_.inReq() - pathState_.inAct());

                // Limit XDV by available. No limit for non-XDV as issuer.
                if (isXDV (node().issue_))
                    node().saFwdDeliver = std::min (
                        node().saFwdDeliver,
                        ledger().accountHolds (
                            node().account_,
                            xdvCurrency(),
                            xdvAccount(),
                            fhIGNORE_FREEZE)); // XDV can't be frozen

            }

            // Record amount sent for pass.
            pathState_.setInPass (node().saFwdDeliver);

            if (!node().saFwdDeliver)
            {
                resultCode   = tecPATH_DRY;
            }
            else if (!isXDV (node().issue_))
            {
                // Non-XDV, current node is the issuer.
                // We could be delivering to multiple accounts, so we don't know
                // which divvy balance will be adjusted.  Assume just issuing.

                WriteLog (lsTRACE, DivvyCalc)
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- !XDV --> offer";

                // As the issuer, would only issue.
                // Don't need to actually deliver. As from delivering leave in
                // the issuer as limbo.
            }
            else
            {
                WriteLog (lsTRACE, DivvyCalc)
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- XDV --> offer";

                // Deliver XDV to limbo.
                resultCode = ledger().accountSend (
                    node().account_, xdvAccount(), node().saFwdDeliver);
            }
        }
    }
    else if (!previousNode().isAccount() && nextNode().isAccount())
    {
        if (nodeIndex_ == lastNodeIndex)
        {
            // offer --> ACCOUNT --> $
            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> $ : "
                << previousNode().saFwdDeliver;

            // Amount to credit.
            pathState_.setOutPass (previousNode().saFwdDeliver);

            // No income balance adjustments necessary.  The paying side inside
            // the offer paid to this account.
        }
        else
        {
            // offer --> ACCOUNT --> account
            WriteLog (lsTRACE, DivvyCalc)
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

            // deliver -> redeem
            if (previousNode().saFwdDeliver && node().saRevRedeem)
                // Previous wants to deliver and can current redeem.
            {
                // Rate : 1.0 : quality out
                divvyLiquidity (
                    divvyCalc_,
                    QUALITY_ONE,
                    uQualityOut,
                    previousNode().saFwdDeliver,
                    node().saRevRedeem,
                    saPrvDeliverAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

            // deliver -> issue
            // Wants to redeem and current would and can issue.
            if (previousNode().saFwdDeliver != saPrvDeliverAct
                // Previous still wants to deliver.
                && node().saRevRedeem == node().saFwdRedeem
                // Current has more to redeem to next.
                && node().saRevIssue)
                // Current wants issue.
            {
                // Rate : 1.0 : transfer_rate
                divvyLiquidity (
                    divvyCalc_,
                    QUALITY_ONE,
                    divvyTransferRate (ledger(), node().account_),
                    previousNode().saFwdDeliver,
                    node().saRevIssue,
                    saPrvDeliverAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            // No income balance adjustments necessary.  The paying side inside
            // the offer paid and the next link will receive.
            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

            if (!saProvide)
                resultCode = tecPATH_DRY;
        }
    }
    else
    {
        // offer --> ACCOUNT --> offer
        // deliver/redeem -> deliver/issue.
        WriteLog (lsTRACE, DivvyCalc)
            << "forwardLiquidityForAccount: offer --> ACCOUNT --> offer";

        node().saFwdDeliver.clear (node().saRevDeliver);

        if (previousNode().saFwdDeliver && node().saRevDeliver)
        {
            // Rate : 1.0 : transfer_rate
            divvyLiquidity (
                divvyCalc_,
                QUALITY_ONE,
                divvyTransferRate (ledger(), node().account_),
                previousNode().saFwdDeliver,
                node().saRevDeliver,
                saPrvDeliverAct,
                node().saFwdDeliver,
                uRateMax);
        }

        // No income balance adjustments necessary.  The paying side inside the
        // offer paid and the next link will receive.
        if (!node().saFwdDeliver)
            resultCode   = tecPATH_DRY;
    }

    return resultCode;
}

} // path
} // divvy