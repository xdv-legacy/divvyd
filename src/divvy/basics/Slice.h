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

#ifndef RIPPLE_BASICS_SLICE_H_INCLUDED
#define RIPPLE_BASICS_SLICE_H_INCLUDED

#include <divvy/basics/strHex.h>
#include <beast/utility/noexcept.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <beast/cxx14/type_traits.h> // <type_traits>

namespace divvy {

/** An immutable linear range of bytes.

    A fully constructed Slice is guaranteed to be in a valid state.
    Default construction, construction from nullptr, and zero-byte
    ranges are disallowed. A Slice is lightweight and copyable, it
    retains no ownership of the underlying memory.
*/
class Slice
{
private:
    std::uint8_t const* data_;
    std::size_t size_;

public:
    // Disallowed
    Slice() = delete;

    Slice (Slice const&) = default;
    
    Slice& operator= (Slice const&) = default;

    /** Create a slice pointing to existing memory. */
    Slice (void const* data, std::size_t size)
        : data_ (reinterpret_cast<
            std::uint8_t const*>(data))
        , size_ (size)
    {
        assert(data_ != nullptr);
        assert(size_ > 0);
    }

    /** Returns the number of bytes in the storage.

        This will never be zero.
    */
    std::size_t
    size() const noexcept
    {
        return size_;
    }

    /** Return a pointer to beginning of the storage.
        @note The return type is guaranteed to be a pointer
              to a single byte, to facilitate pointer arithmetic.
    */
    std::uint8_t const*
    data() const noexcept
    {
        return data_;
    }
};

template <class Hasher>
inline
void
hash_append (Hasher& h, Slice const& v)
{
    h(v.data(), v.size());
}

inline
bool
operator== (Slice const& lhs, Slice const& rhs) noexcept
{
    return lhs.size() == rhs.size() &&
        std::memcmp(
            lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline
bool
operator!= (Slice const& lhs, Slice const& rhs) noexcept
{
    return !(lhs == rhs);
}

inline
bool
operator< (Slice const& lhs, Slice const& rhs) noexcept
{
    return std::lexicographical_compare(
        lhs.data(), lhs.data() + lhs.size(),
            rhs.data(), rhs.data() + rhs.size());
}


template <class Stream>
Stream& operator<<(Stream& s, Slice const& v)
{
    s << strHex(v.data(), v.size());
    return s;
}

template <class T, class Alloc>
std::enable_if_t<
    std::is_same<T, char>::value ||
        std::is_same<T, unsigned char>::value,
    Slice
>
make_Slice (std::vector<T, Alloc> const& v)
{
    return Slice(v.data(), v.size());
}

template <class Traits, class Alloc>
Slice
make_Slice (std::basic_string<char, Traits, Alloc> const& s)
{
    return Slice(s.data(), s.size());
}

std::string
strHex (Slice const& slice);

} // divvy

#endif
