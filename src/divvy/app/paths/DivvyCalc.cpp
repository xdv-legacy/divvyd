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
#include <divvy/app/paths/Tuning.h>
#include <divvy/app/paths/DivvyCalc.h>
#include <divvy/app/paths/cursor/PathCursor.h>
#include <divvy/basics/Log.h>

namespace divvy {
namespace path {

namespace {

TER deleteOffers (
    LedgerEntrySet& activeLedger, OfferSet& offers)
{
    for (auto& o: offers)
    {
        if (TER r = activeLedger.offerDelete (o))
            return r;
    }
    return tesSUCCESS;
}

} // namespace

DivvyCalc::Output DivvyCalc::divvyCalculate (
    LedgerEntrySet& activeLedger,

    // Compute paths using this ledger entry set.  Up to caller to actually
    // apply to ledger.

    // Issuer:
    //      XDV: xdvAccount()
    //  non-XDV: uSrcAccountID (for any issuer) or another account with
    //           trust node.
    STAmount const& saMaxAmountReq,             // --> -1 = no limit.

    // Issuer:
    //      XDV: xdvAccount()
    //  non-XDV: uDstAccountID (for any issuer) or another account with
    //           trust node.
    STAmount const& saDstAmountReq,

    AccountID const& uDstAccountID,
    AccountID const& uSrcAccountID,

    // A set of paths that are included in the transaction that we'll
    // explore for liquidity.
    STPathSet const& spsPaths,
    Input const* const pInputs)
{
    DivvyCalc rc (
        activeLedger,
        saMaxAmountReq,
        saDstAmountReq,
        uDstAccountID,
        uSrcAccountID,
        spsPaths);
    if (pInputs != nullptr)
    {
        rc.inputFlags = *pInputs;
    }

    auto result = rc.divvyCalculate ();
    Output output;
    output.setResult (result);
    output.actualAmountIn = rc.actualAmountIn_;
    output.actualAmountOut = rc.actualAmountOut_;
    output.pathStateList = rc.pathStateList_;

    return output;
}

bool DivvyCalc::addPathState(STPath const& path, TER& resultCode)
{
    auto pathState = std::make_shared<PathState> (
        saDstAmountReq_, saMaxAmountReq_);

    if (!pathState)
    {
        resultCode = temUNKNOWN;
        return false;
    }

    pathState->expandPath (
        mActiveLedger,
        path,
        uDstAccountID_,
        uSrcAccountID_);

    if (pathState->status() == tesSUCCESS)
        pathState->checkNoDivvy (uDstAccountID_, uSrcAccountID_);

    if (pathState->status() == tesSUCCESS)
        pathState->checkFreeze ();

    pathState->setIndex (pathStateList_.size ());

    WriteLog (lsDEBUG, DivvyCalc)
        << "divvyCalc: Build direct:"
        << " status: " << transToken (pathState->status());

    // Return if malformed.
    if (isTemMalformed (pathState->status()))
    {
        resultCode = pathState->status();
        return false;
    }

    if (pathState->status () == tesSUCCESS)
    {
        resultCode = pathState->status();
        pathStateList_.push_back (pathState);
    }
    else if (pathState->status () != terNO_LINE)
    {
        resultCode = pathState->status();
    }

    return true;
}

// OPTIMIZE: When calculating path increment, note if increment consumes all
// liquidity. No need to revisit path in the future if all liquidity is used.

// <-- TER: Only returns tepPATH_PARTIAL if partialPaymentAllowed.
TER DivvyCalc::divvyCalculate ()
{
    assert (mActiveLedger.isValid ());
    WriteLog (lsTRACE, DivvyCalc)
        << "divvyCalc>"
        << " saMaxAmountReq_:" << saMaxAmountReq_
        << " saDstAmountReq_:" << saDstAmountReq_;

    TER resultCode = temUNCERTAIN;
    permanentlyUnfundedOffers_.clear ();
    mumSource_.clear ();

    // YYY Might do basic checks on src and dst validity as per doPayment.

    // Incrementally search paths.
    if (inputFlags.defaultPathsAllowed)
    {
        if (!addPathState (STPath(), resultCode))
            return resultCode;
    }
    else if (spsPaths_.empty ())
    {
        WriteLog (lsDEBUG, DivvyCalc)
            << "divvyCalc: Invalid transaction:"
            << "No paths and direct divvy not allowed.";

        return temRIPPLE_EMPTY;
    }

    // Build a default path.  Use saDstAmountReq_ and saMaxAmountReq_ to imply
    // nodes.
    // XXX Might also make a XDV bridge by default.

    WriteLog (lsTRACE, DivvyCalc)
        << "divvyCalc: Paths in set: " << spsPaths_.size ();

    // Now expand the path state.
    for (auto const& spPath: spsPaths_)
    {
        if (!addPathState (spPath, resultCode))
            return resultCode;
    }

    if (resultCode != tesSUCCESS)
        return (resultCode == temUNCERTAIN) ? terNO_LINE : resultCode;

    resultCode = temUNCERTAIN;

    actualAmountIn_ = saMaxAmountReq_.zeroed();
    actualAmountOut_ = saDstAmountReq_.zeroed();

    // When processing, we don't want to complicate directory walking with
    // deletion.
    const std::uint64_t uQualityLimit = inputFlags.limitQuality ?
            getRate (saDstAmountReq_, saMaxAmountReq_) : 0;

    // Offers that became unfunded.
    OfferSet unfundedOffersFromBestPaths;

    int iPass = 0;

    while (resultCode == temUNCERTAIN)
    {
        int iBest = -1;
        LedgerEntrySet lesCheckpoint = mActiveLedger;
        int iDry = 0;

        // True, if ever computed multi-quality.
        bool multiQuality = false;

        // Find the best path.
        for (auto pathState : pathStateList_)
        {
            if (pathState->quality())
                // Only do active paths.
            {
                // If computing the only non-dry path, compute multi-quality.
                multiQuality = ((pathStateList_.size () - iDry) == 1);

                // Update to current amount processed.
                pathState->reset (actualAmountIn_, actualAmountOut_);

                // Error if done, output met.
                PathCursor pc(*this, *pathState, multiQuality);
                pc.nextIncrement (lesCheckpoint);

                // Compute increment.
                WriteLog (lsDEBUG, DivvyCalc)
                    << "divvyCalc: AFTER:"
                    << " mIndex=" << pathState->index()
                    << " uQuality=" << pathState->quality()
                    << " rate=" << amountFromRate (pathState->quality());

                if (!pathState->quality())
                {
                    // Path was dry.

                    ++iDry;
                }
                else if (pathState->outPass() == zero)
                {
                    // Path is not dry, but moved no funds
                    // This should never happen. Consider the path dry

                    WriteLog (lsWARNING, DivvyCalc)
                        << "rippelCalc: Non-dry path moves no funds";

                    assert (false);

                    pathState->setQuality (0);
                    ++iDry;
                }
                else
                {
                    CondLog (!pathState->inPass() || !pathState->outPass(),
                             lsDEBUG, DivvyCalc)
                        << "divvyCalc: better:"
                        << " uQuality="
                        << amountFromRate (pathState->quality())
                        << " inPass()=" << pathState->inPass()
                        << " saOutPass=" << pathState->outPass();

                    assert (pathState->inPass() && pathState->outPass());

                    if ((!inputFlags.limitQuality ||
                         pathState->quality() <= uQualityLimit)
                        // Quality is not limited or increment has allowed
                        // quality.
                        && (iBest < 0
                            // Best is not yet set.
                            || PathState::lessPriority (
                                *pathStateList_[iBest], *pathState)))
                        // Current is better than set.
                    {
                        WriteLog (lsDEBUG, DivvyCalc)
                            << "divvyCalc: better:"
                            << " mIndex=" << pathState->index()
                            << " uQuality=" << pathState->quality()
                            << " rate="
                            << amountFromRate (pathState->quality())
                            << " inPass()=" << pathState->inPass()
                            << " saOutPass=" << pathState->outPass();

                        assert (mActiveLedger.isValid ());
                        mActiveLedger.swapWith (pathState->ledgerEntries());
                        // For the path, save ledger state.

                        // VFALCO Can this be done without the function call?
                        mActiveLedger.deprecatedInvalidate();

                        iBest   = pathState->index ();
                    }
                }
            }
        }

        ++iPass;
        if (ShouldLog (lsDEBUG, DivvyCalc))
        {
            WriteLog (lsDEBUG, DivvyCalc)
                << "divvyCalc: Summary:"
                << " Pass: " << iPass
                << " Dry: " << iDry
                << " Paths: " << pathStateList_.size ();
            for (auto pathState: pathStateList_)
            {
                WriteLog (lsDEBUG, DivvyCalc)
                    << "divvyCalc: "
                    << "Summary: " << pathState->index()
                    << " rate: "
                    << amountFromRate (pathState->quality())
                    << " quality:" << pathState->quality()
                    << " best: " << (iBest == pathState->index ());
            }
        }

        if (iBest >= 0)
        {
            // Apply best path.
            auto pathState = pathStateList_[iBest];

            WriteLog (lsDEBUG, DivvyCalc)
                << "divvyCalc: best:"
                << " uQuality="
                << amountFromRate (pathState->quality())
                << " inPass()=" << pathState->inPass()
                << " saOutPass=" << pathState->outPass();

            // Record best pass' offers that became unfunded for deletion on
            // success.

            unfundedOffersFromBestPaths.insert (
                pathState->unfundedOffers().begin (),
                pathState->unfundedOffers().end ());

            // Record best pass' LedgerEntrySet to build off of and potentially
            // return.
            assert (pathState->ledgerEntries().isValid ());
            mActiveLedger.swapWith (pathState->ledgerEntries());
            
            // VFALCO Why is this needed? Can it be done
            //        without the function call?
            pathState->ledgerEntries().deprecatedInvalidate();

            actualAmountIn_ += pathState->inPass();
            actualAmountOut_ += pathState->outPass();

            if (pathState->allLiquidityConsumed() || multiQuality)
            {
                ++iDry;
                pathState->setQuality(0);
            }

            if (actualAmountOut_ == saDstAmountReq_)
            {
                // Done. Delivered requested amount.

                resultCode   = tesSUCCESS;
            }
            else if (actualAmountOut_ > saDstAmountReq_)
            {
                WriteLog (lsFATAL, DivvyCalc)
                    << "divvyCalc: TOO MUCH:"
                    << " actualAmountOut_:" << actualAmountOut_
                    << " saDstAmountReq_:" << saDstAmountReq_;

                return tefEXCEPTION;  // TEMPORARY
                assert (false);
            }
            else if (actualAmountIn_ != saMaxAmountReq_ &&
                     iDry != pathStateList_.size ())
            {
                // Have not met requested amount or max send, try to do
                // more. Prepare for next pass.
                //
                // Merge best pass' umReverse.
                mumSource_.insert (
                    pathState->reverse().begin (), pathState->reverse().end ());

                if (iPass >= PAYMENT_MAX_LOOPS)
                {
                    // This payment is taking too many passes

                    WriteLog (lsERROR, DivvyCalc)
                       << "divvyCalc: pass limit";

                    resultCode = telFAILED_PROCESSING;
                }

            }
            else if (!inputFlags.partialPaymentAllowed)
            {
                // Have sent maximum allowed. Partial payment not allowed.

                resultCode   = tecPATH_PARTIAL;
            }
            else
            {
                // Have sent maximum allowed. Partial payment allowed.  Success.

                resultCode   = tesSUCCESS;
            }
        }
        // Not done and ran out of paths.
        else if (!inputFlags.partialPaymentAllowed)
        {
            // Partial payment not allowed.
            resultCode = tecPATH_PARTIAL;
        }
        // Partial payment ok.
        else if (!actualAmountOut_)
        {
            // No payment at all.
            resultCode = tecPATH_DRY;
        }
        else
        {
            // We must restore the activeLedger from lesCheckpoint in the case
            // when iBest is -1 and just before the result is set to tesSUCCESS.

            mActiveLedger.swapWith (lesCheckpoint);
            resultCode   = tesSUCCESS;
        }
    }

    if (resultCode == tesSUCCESS)
    {
        resultCode = deleteOffers(mActiveLedger, unfundedOffersFromBestPaths);
        if (resultCode == tesSUCCESS)
            resultCode = deleteOffers(mActiveLedger, permanentlyUnfundedOffers_);
    }

    // If isOpenLedger, then ledger is not final, can vote no.
    if (resultCode == telFAILED_PROCESSING && !inputFlags.isLedgerOpen)
        return tecFAILED_PROCESSING;
    return resultCode;
}

} // path
} // divvy
