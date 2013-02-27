/*
 * Copyright (C) 2012  Maxim Noah Khailo
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

#ifndef FIRESTR_USER_USER_SERVICE_H
#define FIRESTR_USER_USER_SERVICE_H

#include "user/user.hpp"
#include "service/service.hpp"
#include "network/message_queue.hpp"
#include "util/thread.hpp"

#include <map>
#include <set>
#include <string>
#include <memory>

namespace fire
{
    namespace user
    {
        struct add_request
        {
            std::string to;
            std::string key;
            user_info_ptr from;
        };

        struct contact_data
        {
            enum state { OFFLINE, ONLINE, CONNECTED} state;
            user_info_ptr contact;
            size_t last_ping;
        };

        struct contact_file
        {
            user_info contact;
            std::string greeter;
        };

        bool load_contact_file(const std::string& file, contact_file& cf);
        bool save_contact_file(const std::string& file, const contact_file& cf);

        struct register_with_greeters {};

        typedef std::map<std::string, add_request> add_requests;
        typedef std::set<std::string> sent_requests;
        typedef std::map<std::string, contact_data> contacts_data;

        struct user_service_context
        {
            std::string home;
            std::string host;
            std::string port;
            message::mailbox_ptr events;
        };

        class user_service : public service::service
        {
            public:
                user_service(user_service_context&);
                virtual ~user_service();

            public:
                local_user& user();
                const local_user& user() const;
                const std::string& home() const;
                const std::string& in_host() const;
                const std::string& in_port() const;

            public:
                void attempt_to_add_contact(const std::string& address);
                void add_greeter(const std::string& address);
                void send_confirmation(const std::string& id, std::string key = "");
                void send_rejection(const std::string& id);
                const add_requests& pending_requests() const;

                void confirm_contact(const contact_file&);

            public:
                bool contact_available(const std::string& id) const;

            protected:
                virtual void message_recieved(const message::message&);

            private:
                void update_address(const std::string& address);
                void confirm_contact(user_info_ptr contact);
                void update_contact_address(const std::string& id, const std::string& ip, const std::string& port);
                void find_contact_with_greeter(user_info_ptr c, const std::string& greeter);
                void find_contact(user_info_ptr c);

            private:
                sent_requests _sent_requests;
                add_requests _pending_requests;

            private:
                local_user_ptr _user;

                std::string _home;
                std::mutex _mutex;

            private:
                void fire_new_contact_event(const std::string& id);
                void fire_contact_connected_event(const std::string& id);
                void fire_contact_disconnected_event(const std::string& id);

            private:
                //ping specific 
                void init_ping();
                void init_greet();
                void request_register(const greet_server&);
                void do_regiser_with_greeter(const std::string& greeter);
                void send_ping_requests();
                void send_ping_request(user::user_info_ptr, bool send_back = true);
                void send_ping_request(const std::string& address, user::user_info_ptr, bool send_back = true);
                void send_ping(char t);
                void add_contact_data(user::user_info_ptr);

            private:
                //ping
                mutable std::mutex _ping_mutex;
                util::thread_uptr _ping_thread;
                contacts_data _contacts;

                //greet
                std::string _in_host;
                std::string _in_port;

                bool _done;

            private:
                friend void ping_thread(user_service*);
                friend void greet_thread(user_service*);
        };

        typedef std::shared_ptr<user_service> user_service_ptr;
        typedef std::weak_ptr<user_service> user_service_wptr;

        namespace event
        {
            extern const std::string NEW_CONTACT;
            extern const std::string CONTACT_CONNECTED;
            extern const std::string CONTACT_DISCONNECTED;

            struct new_contact { std::string id; };
            struct contact_connected { std::string id; };
            struct contact_disconnected { std::string id; };

            message::message convert(const new_contact&);
            void convert(const message::message&, new_contact&);

            message::message convert(const contact_connected&);
            void convert(const message::message&, contact_connected&);

            message::message convert(const contact_disconnected&);
            void convert(const message::message&, contact_disconnected&);
        }
    }
}

#endif
