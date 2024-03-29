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

#include <divvy/protocol/STAccount.h>
#include <divvy/protocol/STAmount.h>
#include <divvy/protocol/STArray.h>
#include <divvy/protocol/STBase.h>
#include <divvy/protocol/STBitString.h>
#include <divvy/protocol/STBlob.h>
#include <divvy/protocol/STInteger.h>
#include <divvy/protocol/STObject.h>
#include <divvy/protocol/STPathSet.h>
#include <divvy/protocol/STVector256.h>
#include <divvy/protocol/impl/STVar.h>

namespace divvy {
namespace detail {

defaultObject_t defaultObject;
nonPresentObject_t nonPresentObject;

//------------------------------------------------------------------------------

STVar::Log::~Log()
{
    beast::debug_ostream os;
    for(auto const& e : map_)
        os << e.first << "," << e.second;
}

void
STVar::Log::operator() (std::size_t n)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto const result = map_.emplace(n, 1);
    if (! result.second)
        ++result.first->second;
}

//------------------------------------------------------------------------------

STVar::~STVar()
{
    destroy();
}

STVar::STVar (STVar const& other)
    : p_(nullptr)
{
    if (other.p_ != nullptr)
        p_ = other.p_->copy(
            sizeof(d_), &d_);
}

STVar::STVar (STVar&& other)
{
    if (other.on_heap())
    {
        p_ = other.p_;
        other.p_ = nullptr;
    }
    else
    {
        p_ = other.p_->move(
            sizeof(d_), &d_);
    }
}

STVar&
STVar::operator= (STVar const& rhs)
{
    destroy();
    p_ = nullptr;
    if (rhs.p_)
        p_ = rhs.p_->copy(
            sizeof(d_), &d_);
    return *this;
}

STVar&
STVar::operator= (STVar&& rhs)
{
    destroy();
    if (rhs.on_heap())
    {
        p_ = rhs.p_;
        rhs.p_ = nullptr;
    }
    else
    {
        p_ = nullptr;
        p_ = rhs.p_->move(
            sizeof(d_), &d_);
    }
    return *this;
}

STVar::STVar (defaultObject_t, SField const& name)
    : STVar(name.fieldType, name)
{
}

STVar::STVar (nonPresentObject_t, SField const& name)
    : STVar(STI_NOTPRESENT, name)
{
}

STVar::STVar (SerialIter& sit, SField const& name)
{
    switch (name.fieldType)
    {
    case STI_NOTPRESENT:    construct<STBase>(name); return;
    case STI_UINT8:         construct<STUInt8>(sit, name); return;
    case STI_UINT16:        construct<STUInt16>(sit, name); return;
    case STI_UINT32:        construct<STUInt32>(sit, name); return;
    case STI_UINT64:        construct<STUInt64>(sit, name); return;
    case STI_AMOUNT:        construct<STAmount>(sit, name); return;
    case STI_HASH128:       construct<STHash128>(sit, name); return;
    case STI_HASH160:       construct<STHash160>(sit, name); return;
    case STI_HASH256:       construct<STHash256>(sit, name); return;
    case STI_VECTOR256:     construct<STVector256>(sit, name); return;
    case STI_VL:            construct<STBlob>(sit, name); return;
    case STI_ACCOUNT:       construct<STAccount>(sit, name); return;
    case STI_PATHSET:       construct<STPathSet>(sit, name); return;
    case STI_OBJECT:        construct<STObject>(sit, name); return;
    case STI_ARRAY:         construct<STArray>(sit, name); return;
    default:
        throw std::runtime_error ("Unknown object type");
    }
}

STVar::STVar (SerializedTypeID id, SField const& name)
{
    assert ((id == STI_NOTPRESENT) || (id == name.fieldType));
    switch (id)
    {
    case STI_NOTPRESENT:    construct<STBase>(name); return;
    case STI_UINT8:         construct<STUInt8>(name); return;
    case STI_UINT16:        construct<STUInt16>(name); return;
    case STI_UINT32:        construct<STUInt32>(name); return;
    case STI_UINT64:        construct<STUInt64>(name); return;
    case STI_AMOUNT:        construct<STAmount>(name); return;
    case STI_HASH128:       construct<STHash128>(name); return;
    case STI_HASH160:       construct<STHash160>(name); return;
    case STI_HASH256:       construct<STHash256>(name); return;
    case STI_VECTOR256:     construct<STVector256>(name); return;
    case STI_VL:            construct<STBlob>(name); return;
    case STI_ACCOUNT:       construct<STAccount>(name); return;
    case STI_PATHSET:       construct<STPathSet>(name); return;
    case STI_OBJECT:        construct<STObject>(name); return;
    case STI_ARRAY:         construct<STArray>(name); return;
    default:
        throw std::runtime_error ("Unknown object type");
    }
}

void
STVar::destroy()
{
    if (on_heap())
        delete p_;
    else
        p_->~STBase();
}

} // detail
} // divvy
