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

#ifndef RIPPLE_APP_PEERS_UNIQUENODELIST_H_INCLUDED
#define RIPPLE_APP_PEERS_UNIQUENODELIST_H_INCLUDED

#include <divvy/overlay/ClusterNodeStatus.h>
#include <divvy/protocol/AnyPublicKey.h>
#include <divvy/protocol/DivvyAddress.h>
#include <beast/cxx14/memory.h> // <memory>
#include <beast/threads/Stoppable.h>
#include <boost/filesystem.hpp>

namespace divvy {

class UniqueNodeList : public beast::Stoppable
{
protected:
    explicit UniqueNodeList (Stoppable& parent);

public:
    enum ValidatorSource
    {
        vsConfig    = 'C',  // divvyd.cfg
        vsInbound   = 'I',
        vsManual    = 'M',
        vsReferral  = 'R',
        vsTold      = 'T',
        vsValidator = 'V',  // validators.txt
        vsWeb       = 'W',
    };

    // VFALCO TODO rename this to use the right coding style
    using score = long;

public:
    virtual ~UniqueNodeList () { }

    // VFALCO TODO Roll this into the constructor so there is one less state.
    virtual void start () = 0;

    virtual void insertEphemeralKey (AnyPublicKey pk, std::string comment) = 0;
    virtual void deleteEphemeralKey (AnyPublicKey const& pk) = 0;

    // VFALCO TODO rename all these, the "node" prefix is redundant (lol)
    virtual void nodeAddPublic (DivvyAddress const& naNodePublic, ValidatorSource vsWhy, std::string const& strComment) = 0;
    virtual void nodeAddDomain (std::string strDomain, ValidatorSource vsWhy, std::string const& strComment = "") = 0;
    virtual void nodeRemovePublic (DivvyAddress const& naNodePublic) = 0;
    virtual void nodeRemoveDomain (std::string strDomain) = 0;
    virtual void nodeReset () = 0;

    virtual void nodeScore () = 0;

    virtual bool nodeInUNL (DivvyAddress const& naNodePublic) = 0;
    virtual bool nodeInCluster (DivvyAddress const& naNodePublic) = 0;
    virtual bool nodeInCluster (DivvyAddress const& naNodePublic, std::string& name) = 0;
    virtual bool nodeUpdate (DivvyAddress const& naNodePublic, ClusterNodeStatus const& cnsStatus) = 0;
    virtual std::map<DivvyAddress, ClusterNodeStatus> getClusterStatus () = 0;
    virtual std::uint32_t getClusterFee () = 0;
    virtual void addClusterStatus (Json::Value&) = 0;

    virtual void nodeBootstrap () = 0;
    virtual bool nodeLoad (boost::filesystem::path pConfig) = 0;
    virtual void nodeNetwork () = 0;

    virtual Json::Value getUnlJson () = 0;

    virtual int iSourceScore (ValidatorSource vsWhy) = 0;
};

std::unique_ptr<UniqueNodeList>
make_UniqueNodeList (beast::Stoppable& parent);

} // divvy

#endif
