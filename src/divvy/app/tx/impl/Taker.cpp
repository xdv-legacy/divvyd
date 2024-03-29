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
#include <divvy/app/tx/impl/Taker.h>

namespace divvy {

static
std::string
format_amount (STAmount const& amount)
{
    std::string txt = amount.getText ();
    txt += "/";
    txt += to_string (amount.issue().currency);
    return txt;
}

STAmount
BasicTaker::Rate::divide (STAmount const& amount) const
{
    if (quality_ == QUALITY_ONE)
        return amount;

    return divvy::divide (amount, rate_, amount.issue ());
}

STAmount
BasicTaker::Rate::multiply (STAmount const& amount) const
{
    if (quality_ == QUALITY_ONE)
        return amount;

    return divvy::multiply (amount, rate_, amount.issue ());
}

BasicTaker::BasicTaker (
        CrossType cross_type, AccountID const& account, Amounts const& amount,
        Quality const& quality, std::uint32_t flags, std::uint32_t rate_in,
        std::uint32_t rate_out, beast::Journal journal)
    : account_ (account)
    , quality_ (quality)
    , threshold_ (quality_)
    , sell_ (flags & tfSell)
    , original_ (amount)
    , remaining_ (amount)
    , issue_in_ (remaining_.in.issue ())
    , issue_out_ (remaining_.out.issue ())
    , m_rate_in (rate_in)
    , m_rate_out (rate_out)
    , cross_type_ (cross_type)
    , journal_ (journal)
{
    assert (remaining_.in > zero);
    assert (remaining_.out > zero);

    assert (m_rate_in != 0);
    assert (m_rate_out != 0);

    // If we are dealing with a particular flavor, make sure that it's the
    // flavor we expect:
    assert (cross_type != CrossType::XrpToIou ||
        (isXDV (issue_in ()) && !isXDV (issue_out ())));

    assert (cross_type != CrossType::IouToXrp ||
        (!isXDV (issue_in ()) && isXDV (issue_out ())));

    // And make sure we're not crossing XDV for XDV
    assert (!isXDV (issue_in ()) || !isXDV (issue_out ()));

    // If this is a passive order, we adjust the quality so as to prevent offers
    // at the same quality level from being consumed.
    if (flags & tfPassive)
        ++threshold_;
}

BasicTaker::Rate
BasicTaker::effective_rate (
    std::uint32_t rate, Issue const &issue,
    AccountID const& from, AccountID const& to)
{
    assert (rate != 0);

    if (rate != QUALITY_ONE)
    {
        // We ignore the transfer if the sender is also the recipient since no
        // actual transfer takes place in that case. We also ignore if either
        // the sender or the receiver is the issuer.

        if (from != to && from != issue.account && to != issue.account)
            return Rate (rate);
    }

    return Rate (QUALITY_ONE);
}

bool
BasicTaker::done () const
{
    // We are done if we have consumed all the input currency
    if (remaining_.in <= zero)
    {
        journal_.debug << "Done: all the input currency has been consumed.";
        return true;
    }

    // We are done if using buy semantics and we received the
    // desired amount of output currency
    if (!sell_ && (remaining_.out <= zero))
    {
        journal_.debug << "Done: the desired amount has been received.";
        return true;
    }

    // We are done if the taker is out of funds
    if (get_funds (account(), remaining_.in) <= zero)
    {
        journal_.debug << "Done: taker out of funds.";
        return true;
    }

    return false;
}

Amounts
BasicTaker::remaining_offer () const
{
    // If the taker is done, then there's no offer to place.
    if (done ())
        return Amounts (remaining_.in.zeroed(), remaining_.out.zeroed());

    // Avoid math altogether if we didn't cross.
   if (original_ == remaining_)
       return original_;

    if (sell_)
    {
        assert (remaining_.in > zero);

        // We scale the output based on the remaining input:
        return Amounts (remaining_.in, divRound (
            remaining_.in, quality_.rate (), issue_out_, true));
    }

    assert (remaining_.out > zero);

    // We scale the input based on the remaining output:
    return Amounts (mulRound (
        remaining_.out, quality_.rate (), issue_in_, true), remaining_.out);
}

Amounts const&
BasicTaker::original_offer () const
{
    return original_;
}

// TODO: the presence of 'output' is an artifact caused by the fact that
// Amounts carry issue information which should be decoupled.
static
STAmount
qual_div (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = divide (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

static
STAmount
qual_mul (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = multiply (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

void
BasicTaker::log_flow (char const* description, Flow const& flow)
{
    if (!journal_.debug)
        return;

    journal_.debug << description;

    if (isXDV (issue_in ()))
        journal_.debug << "   order in: " << format_amount (flow.order.in);
    else
        journal_.debug << "   order in: " << format_amount (flow.order.in) <<
            " (issuer: " << format_amount (flow.issuers.in) << ")";

    if (isXDV (issue_out ()))
        journal_.debug << "  order out: " << format_amount (flow.order.out);
    else
        journal_.debug << "  order out: " << format_amount (flow.order.out) <<
            " (issuer: " << format_amount (flow.issuers.out) << ")";
}

BasicTaker::Flow
BasicTaker::flow_xdv_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.out = rate_out.multiply (f.order.out);

    log_flow ("flow_xdv_to_iou", f);

    // Clamp on owner balance
    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = rate_out.divide (f.issuers.out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        log_flow ("(clamped on owner balance)", f);
    }

    // Clamp if taker wants to limit the output
    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = rate_out.multiply (f.order.out);
        log_flow ("(clamped on taker output)", f);
    }

    // Clamp on the taker's funds
    if (taker_funds < f.order.in)
    {
        f.order.in = taker_funds;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = rate_out.multiply (f.order.out);
        log_flow ("(clamped on taker funds)", f);
    }

    // Clamp on remaining offer if we are not handling the second leg
    // of an autobridge.
    if (cross_type_ == CrossType::XrpToIou && (remaining_.in < f.order.in))
    {
        f.order.in = remaining_.in;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = rate_out.multiply (f.order.out);
        log_flow ("(clamped on taker input)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_xdv (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in)
{
    Flow f;
    f.order = order;
    f.issuers.in = rate_in.multiply (f.order.in);

    log_flow ("flow_iou_to_xdv", f);

    // Clamp on owner's funds
    if (owner_funds < f.order.out)
    {
        f.order.out = owner_funds;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = rate_in.multiply (f.order.in);
        log_flow ("(clamped on owner funds)", f);
    }

    // Clamp if taker wants to limit the output and we are not the
    // first leg of an autobridge.
    if (!sell_ && cross_type_ == CrossType::IouToXrp)
    {
        if (remaining_.out < f.order.out)
        {
            f.order.out = remaining_.out;
            f.order.in = qual_mul (f.order.out, quality, f.order.in);
            f.issuers.in = rate_in.multiply (f.order.in);
            log_flow ("(clamped on taker output)", f);
        }
    }

    // Clamp on the taker's input offer
    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = rate_in.multiply (f.order.in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker input)", f);
    }

    // Clamp on the taker's input balance
    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = rate_in.divide (f.issuers.in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in, Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.in = rate_in.multiply (f.order.in);
    f.issuers.out = rate_out.multiply (f.order.out);

    log_flow ("flow_iou_to_iou", f);

    // Clamp on owner balance
    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = rate_out.divide (f.issuers.out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = rate_in.multiply (f.order.in);
        log_flow ("(clamped on owner funds)", f);
    }

    // Clamp on taker's offer
    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = rate_out.multiply (f.order.out);
        f.issuers.in = rate_in.multiply (f.order.in);
        log_flow ("(clamped on taker output)", f);
    }

    // Clamp on the taker's input offer
    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = rate_in.multiply (f.order.in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = rate_out.multiply (f.order.out);
        log_flow ("(clamped on taker input)", f);
    }

    // Clamp on the taker's input balance
    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = rate_in.divide (f.issuers.in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = rate_out.multiply (f.order.out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

// Calculates the direct flow through the specified offer
BasicTaker::Flow
BasicTaker::do_cross (Amounts offer, Quality quality, AccountID const& owner)
{
    assert (!done ());

    auto const owner_funds = get_funds (owner, offer.out);
    auto const taker_funds = get_funds (account (), offer.in);

    Flow result;

    if (cross_type_ == CrossType::XrpToIou)
    {
        result = flow_xdv_to_iou (offer, quality, owner_funds, taker_funds,
            out_rate (owner, account ()));
    }
    else if (cross_type_ == CrossType::IouToXrp)
    {
        result = flow_iou_to_xdv (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()));
    }
    else
    {
        result = flow_iou_to_iou (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()), out_rate (owner, account ()));
    }

    if (!result.sanity_check ())
        throw std::logic_error ("Computed flow fails sanity check.");

    remaining_.out -= result.order.out;
    remaining_.in -= result.order.in;

    assert (remaining_.in >= zero);

    return result;
}

// Calculates the bridged flow through the specified offers
std::pair<BasicTaker::Flow, BasicTaker::Flow>
BasicTaker::do_cross (
    Amounts offer1, Quality quality1, AccountID const& owner1,
    Amounts offer2, Quality quality2, AccountID const& owner2)
{
    assert (!done ());

    assert (!offer1.in.native ());
    assert (offer1.out.native ());
    assert (offer2.in.native ());
    assert (!offer2.out.native ());

    // If the taker owns the first leg of the offer, then the taker's available
    // funds aren't the limiting factor for the input - the offer itself is.
    auto leg1_in_funds = get_funds (account (), offer1.in);

    if (account () == owner1)
    {
        journal_.trace << "The taker owns the first leg of a bridge.";
        leg1_in_funds = std::max (leg1_in_funds, offer1.in);
    }

    // If the taker owns the second leg of the offer, then the taker's available
    // funds are not the limiting factor for the output - the offer itself is.
    auto leg2_out_funds = get_funds (owner2, offer2.out);

    if (account () == owner2)
    {
        journal_.trace << "The taker owns the second leg of a bridge.";
        leg2_out_funds = std::max (leg2_out_funds, offer2.out);
    }

    // The amount available to flow via XDV is the amount that the owner of the
    // first leg of the bridge has, up to the first leg's output.
    //
    // But, when both legs of a bridge are owned by the same person, the amount
    // of XDV that can flow between the two legs is, essentially, infinite
    // since all the owner is doing is taking out XDV of his left pocket
    // and putting it in his right pocket. In that case, we set the available
    // XDV to the largest of the two offers.
    auto xdv_funds = get_funds (owner1, offer1.out);

    if (owner1 == owner2)
    {
        journal_.trace << "The bridge endpoints are owneb by the same account.";
        xdv_funds = std::max (offer1.out, offer2.in);
    }

    if (journal_.debug)
    {
        journal_.debug << "Available bridge funds:";
        journal_.debug << "  leg1 in: " << format_amount (leg1_in_funds);
        journal_.debug << " leg2 out: " << format_amount (leg2_out_funds);
        journal_.debug << "      xdv: " << format_amount (xdv_funds);
    }

    auto const leg1_rate = in_rate (owner1, account ());
    auto const leg2_rate = out_rate (owner2, account ());

    // Attempt to determine the maximal flow that can be achieved across each
    // leg independent of the other.
    auto flow1 = flow_iou_to_xdv (offer1, quality1, xdv_funds, leg1_in_funds, leg1_rate);

    if (!flow1.sanity_check ())
        throw std::logic_error ("Computed flow1 fails sanity check.");

    auto flow2 = flow_xdv_to_iou (offer2, quality2, leg2_out_funds, xdv_funds, leg2_rate);

    if (!flow2.sanity_check ())
        throw std::logic_error ("Computed flow2 fails sanity check.");

    // We now have the maximal flows across each leg individually. We need to
    // equalize them, so that the amount of XDV that flows out of the first leg
    // is the same as the amount of XDV that flows into the second leg. We take
    // the side which is the limiting factor (if any) and adjust the other.
    if (flow1.order.out < flow2.order.in)
    {
        // Adjust the second leg of the offer down:
        flow2.order.in = flow1.order.out;
        flow2.order.out = qual_div (flow2.order.in, quality2, flow2.order.out);
        flow2.issuers.out = leg2_rate.multiply (flow2.order.out);
        log_flow ("Balancing: adjusted second leg down", flow2);
    }
    else if (flow1.order.out > flow2.order.in)
    {
        // Adjust the first leg of the offer down:
        flow1.order.out = flow2.order.in;
        flow1.order.in = qual_mul (flow1.order.out, quality1, flow1.order.in);
        flow1.issuers.in = leg1_rate.multiply (flow1.order.in);
        log_flow ("Balancing: adjusted first leg down", flow2);
    }

    if (flow1.order.out != flow2.order.in)
        throw std::logic_error ("Bridged flow is out of balance.");

    remaining_.out -= flow2.order.out;
    remaining_.in -= flow1.order.in;

    return std::make_pair (flow1, flow2);
}

//==============================================================================

std::uint32_t
Taker::calculateRate (
    LedgerView& view, AccountID const& issuer, AccountID const& account)
{
    return isXDV (issuer) || (account == issuer)
        ? QUALITY_ONE
        : divvyTransferRate (view, issuer);
}

Taker::Taker (CrossType cross_type, LedgerView& view, AccountID const& account,
        Amounts const& offer, std::uint32_t flags, beast::Journal journal)
    : BasicTaker (cross_type, account, offer, Quality(offer), flags,
        calculateRate(view, offer.in.getIssuer(), account),
        calculateRate(view, offer.out.getIssuer(), account), journal)
    , view_ (view)
    , xdv_flow_ (0)
    , direct_crossings_ (0)
    , bridge_crossings_ (0)
{
    assert (issue_in () == offer.in.issue ());
    assert (issue_out () == offer.out.issue ());

    if (journal_.debug)
    {
        journal_.debug << "Crossing as: " << to_string (account);

        if (isXDV (issue_in ()))
            journal_.debug << "   Offer in: " << format_amount (offer.in);
        else
            journal_.debug << "   Offer in: " << format_amount (offer.in) <<
                " (issuer: " << issue_in ().account << ")";

        if (isXDV (issue_out ()))
            journal_.debug << "  Offer out: " << format_amount (offer.out);
        else
            journal_.debug << "  Offer out: " << format_amount (offer.out) <<
                " (issuer: " << issue_out ().account << ")";

        journal_.debug <<
            "    Balance: " << format_amount (get_funds (account, offer.in));
    }
}

void
Taker::consume_offer (Offer const& offer, Amounts const& order)
{
    if (order.in < zero)
        throw std::logic_error ("flow with negative input.");

    if (order.out < zero)
        throw std::logic_error ("flow with negative output.");

    if (journal_.debug) journal_.debug << "Consuming from offer " << offer;

    if (journal_.trace)
    {
        auto const& available = offer.amount ();

        journal_.trace << "   in:" << format_amount (available.in);
        journal_.trace << "  out:" << format_amount(available.out);
    }

    offer.consume (view_, order);
}

STAmount
Taker::get_funds (AccountID const& account, STAmount const& amount) const
{
    return funds(view_, account, amount, fhZERO_IF_FROZEN);
}

TER Taker::transfer_xdv (
    AccountID const& from,
    AccountID const& to,
    STAmount const& amount)
{
    if (!isXDV (amount))
        throw std::logic_error ("Using transfer_xdv with IOU");

    if (from == to)
        return tesSUCCESS;

    // Transferring zero is equivalent to not doing a transfer
    if (amount == zero)
        return tesSUCCESS;

    return view_.transfer_xdv (from, to, amount);
}

TER Taker::redeem_iou (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXDV (amount))
        throw std::logic_error ("Using redeem_iou with XDV");

    if (account == issue.account)
        return tesSUCCESS;

    // Transferring zero is equivalent to not doing a transfer
    if (amount == zero)
        return tesSUCCESS;

    // If we are trying to redeem some amount, then the account
    // must have a credit balance.
    if (get_funds (account, amount) <= zero)
        throw std::logic_error ("redeem_iou has no funds to redeem");

    auto ret = view_.redeem_iou (account, amount, issue);

    if (get_funds (account, amount) < zero)
        throw std::logic_error ("redeem_iou redeemed more funds than available");

    return ret;
}

TER Taker::issue_iou (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXDV (amount))
        throw std::logic_error ("Using issue_iou with XDV");

    if (account == issue.account)
        return tesSUCCESS;

    // Transferring zero is equivalent to not doing a transfer
    if (amount == zero)
        return tesSUCCESS;

    return view_.issue_iou (account, amount, issue);
}

// Performs funds transfers to fill the given offer and adjusts offer.
TER
Taker::fill (BasicTaker::Flow const& flow, Offer const& offer)
{
    // adjust offer
    consume_offer (offer, flow.order);

    TER result = tesSUCCESS;

    if (cross_type () != CrossType::XrpToIou)
    {
        assert (!isXDV (flow.order.in));

        if(result == tesSUCCESS)
            result = redeem_iou (account (), flow.issuers.in, flow.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issue_iou (offer.owner (), flow.order.in, flow.order.in.issue ());
    }
    else
    {
        assert (isXDV (flow.order.in));

        if (result == tesSUCCESS)
            result = transfer_xdv (account (), offer.owner (), flow.order.in);
    }

    // Now send funds from the account whose offer we're taking
    if (cross_type () != CrossType::IouToXrp)
    {
        assert (!isXDV (flow.order.out));

        if(result == tesSUCCESS)
            result = redeem_iou (offer.owner (), flow.issuers.out, flow.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issue_iou (account (), flow.order.out, flow.order.out.issue ());
    }
    else
    {
        assert (isXDV (flow.order.out));

        if (result == tesSUCCESS)
            result = transfer_xdv (offer.owner (), account (), flow.order.out);
    }

    if (result == tesSUCCESS)
        direct_crossings_++;

    return result;
}

// Performs bridged funds transfers to fill the given offers and adjusts offers.
TER
Taker::fill (
    BasicTaker::Flow const& flow1, Offer const& leg1,
    BasicTaker::Flow const& flow2, Offer const& leg2)
{
    // Adjust offers accordingly
    consume_offer (leg1, flow1.order);
    consume_offer (leg2, flow2.order);

    TER result = tesSUCCESS;

    // Taker to leg1: IOU
    if (leg1.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeem_iou (account (), flow1.issuers.in, flow1.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issue_iou (leg1.owner (), flow1.order.in, flow1.order.in.issue ());
    }

    // leg1 to leg2: bridging over XDV
    if (result == tesSUCCESS)
        result = transfer_xdv (leg1.owner (), leg2.owner (), flow1.order.out);

    // leg2 to Taker: IOU
    if (leg2.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeem_iou (leg2.owner (), flow2.issuers.out, flow2.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issue_iou (account (), flow2.order.out, flow2.order.out.issue ());
    }

    if (result == tesSUCCESS)
    {
        bridge_crossings_++;
        xdv_flow_ += flow1.order.out;
    }

    return result;
}

TER
Taker::cross (Offer const& offer)
{
    // In direct crossings, at least one leg must not be XDV.
    if (isXDV (offer.amount ().in) && isXDV (offer.amount ().out))
        return tefINTERNAL;

    auto const amount = do_cross (
        offer.amount (), offer.quality (), offer.owner ());

    return fill (amount, offer);
}

TER
Taker::cross (Offer const& leg1, Offer const& leg2)
{
    // In bridged crossings, XDV must can't be the input to the first leg
    // or the output of the second leg.
    if (isXDV (leg1.amount ().in) || isXDV (leg2.amount ().out))
        return tefINTERNAL;

    auto ret = do_cross (
        leg1.amount (), leg1.quality (), leg1.owner (),
        leg2.amount (), leg2.quality (), leg2.owner ());

    return fill (ret.first, leg1, ret.second, leg2);
}

}
