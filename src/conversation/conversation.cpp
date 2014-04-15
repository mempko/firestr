/*
 * Copyright (C) 2014  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "conversation/conversation.hpp"

#include "util/uuid.hpp"
#include "util/dbc.hpp"

namespace u = fire::util;
namespace us = fire::user;
namespace m = fire::message;
namespace ms = fire::messages;

namespace fire
{
    namespace conversation
    {
        namespace
        {
            const std::string SCRIPT_APP = "SCRIPT_APP";
        }

        conversation::conversation(
                us::user_service_ptr s,
                m::post_office_wptr pp) :
            _id{u::uuid()},
            _user_service{s},
            _parent_post{pp},
            _initiated_by_user{true}
        {
            init();
        }

        conversation::conversation(
                const std::string id, 
                us::user_service_ptr s,
                m::post_office_wptr pp) :
            _id{id},
            _user_service{s},
            _parent_post{pp},
            _initiated_by_user{false}
        {
            init();
        }

        void conversation::init()
        {
            INVARIANT_FALSE(_id.empty());
            INVARIANT(_user_service);

            _mail = std::make_shared<m::mailbox>(_id);
            _sender = std::make_shared<ms::sender>(_user_service, _mail);

            INVARIANT(_mail);
            INVARIANT(_sender);
        }

        const user::contact_list& conversation::contacts() const
        {
            return _contacts;
        }

        user::contact_list& conversation::contacts() 
        {
            return _contacts;
        }

        const std::string& conversation::id() const
        {
            ENSURE_FALSE(_id.empty());
            return _id;
        }

        bool conversation::initiated_by_user() const
        {
            return _initiated_by_user;
        }

        void conversation::initiated_by_user(bool v)
        {
            _initiated_by_user = v;
        }

        m::post_office_wptr conversation::parent_post()
        {
            return _parent_post;
        }

        m::mailbox_ptr conversation::mail()
        {
            ENSURE(_mail);
            return _mail;
        }

        messages::sender_ptr conversation::sender()
        {
            ENSURE(_sender);
            return _sender;
        }

        user::user_service_ptr conversation::user_service()
        {
            ENSURE(_user_service);
            return _user_service;
        }

        const app_metadata& conversation::apps() const
        {
            return _app_metadata;
        }

        bool conversation::has_app(const std::string& address) const
        {
            return _app_addresses.count(address) > 0;
        }

        void conversation::add_app(const app_metadatum& d)
        {
            REQUIRE(d.type != SCRIPT_APP || !d.id.empty());
            REQUIRE_FALSE(d.type.empty());
            REQUIRE_FALSE(d.address.empty());
            _app_metadata.push_back(d);
            _app_addresses.insert(d.address);
        }

        bool conversation::send(const std::string& to, const message::message& m)
        {
            INVARIANT(_sender);
            return _sender->send(to, m);
        }

        bool conversation::send(const message::message& m)
        {
            bool sent = false;
            for(auto c : _contacts.list())
            {
                CHECK(c);
                sent |= send(c->id(), m); 
            }
            return sent;
        }
    }
}

