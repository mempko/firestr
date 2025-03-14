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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * Botan library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than Botan. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#include "conversation/conversation.hpp"

#include <algorithm>
#include "util/dbc.hpp"
#include "util/log.hpp"
#include "util/uuid.hpp"

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
            _id(u::uuid()),
            _parent_post{pp},
            _user_service{s},
            _initiated_by_user{true}
        {
            init();
        }

        conversation::conversation(
                const std::string id, 
                us::user_service_ptr s,
                m::post_office_wptr pp) :
            _id(id),
            _parent_post{pp},
            _user_service{s},
            _initiated_by_user{false}
        {
            init();
        }

        conversation::~conversation()
        {
            INVARIANT(_mail);
            if(auto p = _parent_post.lock())
            {
                LOG << "removing mailbox " << _mail->address() << std::endl;
                p->remove_mailbox(_mail->address());
            }
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

        const pending_contact_adds& conversation::pending() const
        {
            return _pending_adds;
        }

        pending_contact_adds& conversation::pending()
        {
            return _pending_adds;
        }

        void conversation::asked_about(const std::string& id) 
        {
            auto& a = _pending_adds[id];
            a.requests.clear();
            a.contact.id = id;
            
            for(auto c : _contacts.list())
            {
                CHECK(c);
                auto& req = a.requests[c->id()];
                req.state = know_request::SENT;
                req.to = c->id();
            }
        }

        void conversation::know_contact(
                const std::string& id, 
                const std::string& from, 
                know_request::req_state s)
        {
            auto pi = _pending_adds.find(id);
            if(pi == _pending_adds.end()) return;

            auto ri = pi->second.requests.find(from);
            if(ri == pi->second.requests.end()) return;

            ri->second.state = s;
        }

        clique_status conversation::part_of_clique(std::string& id)
        {
            clique_status r;
            r.is_part = clique_status::DONT_KNOW;

            //find pending add
            auto pi = _pending_adds.find(id);
            if(pi == _pending_adds.end()) return r;

            //get requests. 
            const auto& requests = pi->second.requests;
            if(requests.size() != _contacts.size()) return r;

            //count how many requests are in sent state
            size_t how_many_sent = 
                std::count_if(
                    requests.begin(), requests.end(),
                    [](const know_requests::value_type& p)
                    { 
                        return p.second.state == know_request::SENT;
                    });

            //if we still have sent requests without responses,
            //we still don't know
            if(how_many_sent > 0) return r;

            //count how many requests are in know state
            size_t how_many_know = 
                std::count_if(
                    requests.begin(), requests.end(),
                    [](const know_requests::value_type& p)
                    { 
                        return p.second.state == know_request::KNOW;
                    });

            //if everyone knows the contact, then they are part of clique
            if(how_many_know == requests.size())
            {
                r.is_part = clique_status::PART;
                return r;
            }

            //otherwise they are not part of clique and we need to collect
            //the ids of the contacts who don't know them
            r.is_part = clique_status::NOT_PART;
            for(const auto& req : requests)
                if(req.second.state == know_request::DONT_KNOW)
                    r.contacts.insert(r.contacts.end(), req.second.to);

            return r;
        }

        void conversation::remove_contact(std::string& id)
        {
            auto c = _contacts.by_id(id);
            if(c) 
            {
                _contacts.remove(c);
                _removed.insert(id);
            }

            auto pi = _pending_adds.find(id);
            if(pi != _pending_adds.end()) _pending_adds.erase(pi);
        }

        void conversation::add_contact(const us::user_info_ptr c)
        {
            REQUIRE(c);
            _contacts.add(c);
            _removed.erase(c->id());
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
            return _app_metadata.count(address) > 0;
        }

        void conversation::add_app(const app_metadatum& d)
        {
            REQUIRE(d.type != SCRIPT_APP || !d.id.empty());
            REQUIRE_FALSE(d.type.empty());
            REQUIRE_FALSE(d.address.empty());
            _app_metadata[d.address] = d;
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

