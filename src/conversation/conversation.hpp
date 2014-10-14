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

#ifndef FIRESTR_CONVERSATION_CONVERSATION_H
#define FIRESTR_CONVERSATION_CONVERSATION_H

#include "user/user.hpp"
#include "user/userservice.hpp"

#include "message/postoffice.hpp"
#include "message/mailbox.hpp"
#include "messages/sender.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace fire
{
    namespace conversation
    {
        struct app_metadatum
        {
            std::string type;
            std::string id;
            std::string address;
        };

        using app_metadata = std::map<std::string, app_metadatum>;

        struct know_request
        {
            std::string to;
            enum req_state { SENT, DONT_KNOW, KNOW} state; 
        };

        using know_requests = std::unordered_map<std::string, know_request>;

        struct pending_contact
        {
            std::string id;
            std::string from;
            std::string name; //this information will eventually be used for guests
        };

        struct pending_contact_add
        {
            pending_contact contact;
            know_requests requests; 
        };

        using contact_id_set = std::set<std::string>;
        struct clique_status
        {
            enum status { NOT_PART, PART, DONT_KNOW} is_part;
            contact_id_set contacts;
        };

        using pending_contact_adds = std::unordered_map<std::string, pending_contact_add>;

        class conversation 
        {
            public:
                conversation(user::user_service_ptr, message::post_office_wptr);
                conversation(const std::string id, user::user_service_ptr, message::post_office_wptr);
                ~conversation();

            public:
                const user::contact_list& contacts() const;
                user::contact_list& contacts(); 

            public:
                bool send(const std::string& to, const message::message& m);
                bool send(const message::message&);

            public:
                const pending_contact_adds& pending() const;
                pending_contact_adds& pending();

                void asked_about(const std::string& id);

                void know_contact(
                        const std::string& id, 
                        const std::string& from, 
                        know_request::req_state);

                clique_status part_of_clique(std::string& id);
                void remove_contact(std::string& id);

            public:
                const std::string& id() const;
                bool initiated_by_user() const;
                void initiated_by_user(bool);

            public:
                message::post_office_wptr parent_post();
                message::mailbox_ptr mail();
                messages::sender_ptr sender();
                user::user_service_ptr user_service();

            public:
                const app_metadata& apps() const;
                bool has_app(const std::string& address) const;
                void add_app(const app_metadatum&);

            private:
                void init();

            private:
                std::string _id;
                message::post_office_wptr _parent_post;
                message::mailbox_ptr _mail;
                user::user_service_ptr _user_service;
                messages::sender_ptr _sender;
                user::contact_list _contacts;
                app_metadata _app_metadata;
                pending_contact_adds _pending_adds;
                bool _initiated_by_user;
        };

        using conversation_ptr = std::shared_ptr<conversation>;
        using conversation_wptr = std::weak_ptr<conversation>;
    }
}

#endif

