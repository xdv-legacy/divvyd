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
#include <divvy/nodestore/impl/ManagerImp.h>
#include <divvy/nodestore/impl/DatabaseImp.h>
#include <divvy/nodestore/impl/DatabaseRotatingImp.h>
#include <divvy/basics/StringUtilities.h>
#include <beast/utility/ci_char_traits.h>
#include <beast/cxx14/memory.h> // <memory>
#include <stdexcept>

namespace divvy {
namespace NodeStore {

ManagerImp&
ManagerImp::instance()
{
    static beast::static_initializer<ManagerImp> _;
    return _.get();
}

void
ManagerImp::missing_backend()
{
    throw std::runtime_error (
        "Your divvyd.cfg is missing a [node_db] entry, "
        "please see the divvyd-example.cfg file!"
        );
}

ManagerImp::ManagerImp()
{
}

ManagerImp::~ManagerImp()
{
}

std::unique_ptr <Backend>
ManagerImp::make_Backend (
    Section const& parameters,
    Scheduler& scheduler,
    beast::Journal journal)
{
    std::unique_ptr <Backend> backend;

    std::string const type (get<std::string>(parameters, "type"));

    if (! type.empty ())
    {
        Factory* const factory (find (type));

        if (factory != nullptr)
        {
            backend = factory->createInstance (
                NodeObject::keyBytes, parameters, scheduler, journal);
        }
        else
        {
            missing_backend ();
        }
    }
    else
    {
        missing_backend ();
    }

    return backend;
}

std::unique_ptr <Database>
ManagerImp::make_Database (
    std::string const& name,
    Scheduler& scheduler,
    beast::Journal journal,
    int readThreads,
    Section const& backendParameters)
{
    std::unique_ptr <Backend> backend (make_Backend (
        backendParameters, scheduler, journal));

    return std::make_unique <DatabaseImp> (name, scheduler, readThreads,
        std::move (backend), journal);
}

std::unique_ptr <DatabaseRotating>
ManagerImp::make_DatabaseRotating (
        std::string const& name,
        Scheduler& scheduler,
        std::int32_t readThreads,
        std::shared_ptr <Backend> writableBackend,
        std::shared_ptr <Backend> archiveBackend,
        beast::Journal journal)
{
    return std::make_unique <DatabaseRotatingImp> (name, scheduler,
            readThreads, writableBackend, archiveBackend, journal);
}

Factory*
ManagerImp::find (std::string const& name)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&name](Factory* other)
        {
            return beast::ci_equal(name, other->getName());
        } );
    if (iter == list_.end())
        return nullptr;
    return *iter;
}


void
ManagerImp::insert (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    list_.push_back(&factory);
}

void
ManagerImp::erase (Factory& factory)
{
    std::lock_guard<std::mutex> _(mutex_);
    auto const iter = std::find_if(list_.begin(), list_.end(),
        [&factory](Factory* other) { return other == &factory; });
    assert(iter != list_.end());
    list_.erase(iter);
}

//------------------------------------------------------------------------------

Manager&
Manager::instance()
{
    return ManagerImp::instance();
}

//------------------------------------------------------------------------------

std::unique_ptr <Backend>
make_Backend (Section const& config,
    Scheduler& scheduler, beast::Journal journal)
{
    return Manager::instance().make_Backend (
        config, scheduler, journal);
}

}
}
