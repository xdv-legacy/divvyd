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
#include <divvy/basics/BasicConfig.h>
#include <divvy/websocket/MakeServer.h>
#include <divvy/websocket/WebSocket.h>

namespace divvy {
namespace websocket {

std::unique_ptr<beast::Stoppable> makeServer (ServerDescription const& desc)
{
    auto version = get<std::string> (
        desc.config["server"], "websocket_version");
    if (version.empty())
        version = WebSocket02::versionName();

    WriteLog (lsWARNING, WebSocket) << "Websocket version " << version;
    if (version == WebSocket02::versionName())
        return makeServer02 (desc);
    assert (version == "04");
    return makeServer04 (desc);
}

} // websocket
} // divvy
