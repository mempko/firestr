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

#ifndef FIRESTR_USER_USER_SERVICE_H
#define FIRESTR_USER_USER_SERVICE_H

#include "user/user.hpp"
#include "service/service.hpp"
#include "security/security_library.hpp"
#include "util/thread.hpp"
#include "util/mencode.hpp"

#include <map>
#include <set>
#include <string>
#include <memory>

namespace fire
{
    namespace user
    {
        struct contact_data
        {
            enum state { OFFLINE, CONNECTING, CONNECTED} state;
            user_info_ptr contact;
            size_t last_ping;
            int client_version;
            int protocol_version;
        };

        struct contact_version
        {
            int protocol;
            int client;
        };

        struct identity
        {
            user_info contact;
            std::string greeter;
        };

        bool parse_identity(const std::string& iden64, identity& cf);
        std::string create_identity(const identity& cf);

        struct register_with_greeters {};

        using contacts_data = std::map<std::string, contact_data>;

        struct user_service_context
        {
            std::string home;
            std::string host;
            network::port_type port;
            local_user_ptr user;
            message::mailbox_ptr events;
            security::encrypted_channels_ptr encrypted_channels;
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
                network::port_type in_port() const;

            public:
                bool confirm_contact(const identity&);
                void remove_contact(const std::string& id);

                void add_greeter(const std::string& address);
                void remove_greeter(const std::string& address);

                void remove_introduction(const contact_introduction&);
                contact_introductions introductions() const;
                void send_introduction(const std::string& to, contact_introduction&);

            public:
                user_info_ptr by_id(const std::string& id) const;
                bool contact_available(const std::string& id) const;
                int  protocol_version(const std::string& id) const;
                contact_version check_contact_version(const std::string& id) const;

            protected:
                void received_ping(const message::message& m);
                void received_connect_request(const message::message& m);
                void received_register_with_greeter(const message::message& m);
                void received_greet_key_response(const message::message& m);
                void received_greet_find_response(const message::message& m);
                void received_introduction(const message::message& m);

            private:
                int add_introduction(const contact_introduction&);
                void add_greeter(const std::string& host, network::port_type port, const std::string& pub_key);
                void update_address(const std::string& address);
                void update_contact_address(const std::string& id, const std::string& ip, network::port_type port);
                void update_contact_version( const std::string& id, int protocol_version, int client_version);
                void find_contact_with_greeter(user_info_ptr c, const std::string& greeter);

            private:
                local_user_ptr _user;

                std::string _home;
                mutable std::mutex _mutex;

            private:
                bool is_contact_connecting(const std::string& id) const;
                bool contact_connecting(const std::string& id);
                bool contact_connected(const std::string& id);
                void fire_contact_connected_event(const std::string& id);
                void fire_contact_disconnected_event(const std::string& id);
                void fire_new_introduction(size_t i);

            private:
                void init_handlers();

                //ping specific 
                void init_ping();
                void init_reconnect();
                void reconnect();
                void request_register(const greet_server&);
                void do_regiser_with_greeter(
                        const std::string& tcp_addr, 
                        const std::string& udp_addr, 
                        const std::string& pub_key);

                void send_ping_requests();
                void send_ping_request(user::user_info_ptr, bool send_back = true);
                void send_ping_request(const std::string& address, bool send_back = true);
                void send_ping(char t);
                void send_ping_to(char t, const std::string& id, bool force = false);
                void add_contact_data(user::user_info_ptr);
                void setup_security_conversation(
                        const std::string& address, 
                        const security::public_key& key, 
                        const util::bytes& public_val);

            private:
                //ping
                mutable std::mutex _ping_mutex;
                util::thread_uptr _ping_thread;
                util::thread_uptr _reconnect_thread;
                contacts_data _contacts;

                //greet
                std::string _in_host;
                network::port_type _in_port;

                //stores conversation security information for 
                //a connection with a user
                security::encrypted_channels_ptr _encrypted_channels;

                bool _done;

            private:
                friend void ping_thread(user_service*);
                friend void greet_thread(user_service*);
                friend void reconnect_thread(user_service*);
        };

        using user_service_ptr = std::shared_ptr<user_service>;
        using user_service_wptr = std::weak_ptr<user_service>;

        namespace event
        {
            extern const std::string CONTACT_CONNECTED;
            extern const std::string CONTACT_DISCONNECTED;
            extern const std::string NEW_INTRODUCTION;

            f_message(contact_connected)
            { 
                std::string id; 

                f_message_init(contact_connected, CONTACT_CONNECTED);
                f_serialize 
                { 
                    f_s(id); 
                }
            };

            f_message(contact_disconnected)
            { 
                std::string id; 
                std::string name;

                f_message_init(contact_disconnected, CONTACT_DISCONNECTED);
                f_serialize 
                { 
                    f_s(id); 
                    f_s(name); 
                }
            };

            f_message(new_introduction)
            { 
                size_t index;

                f_message_init(new_introduction, NEW_INTRODUCTION);
                f_serialize
                {
                    f_s(index);
                }
            };

        }
    }
}

#endif

