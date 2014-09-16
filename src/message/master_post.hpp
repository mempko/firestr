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
#ifndef FIRESTR_MESSAGE_MASERT_POSTOFFICE_H
#define FIRESTR_MESSAGE_MASERT_POSTOFFICE_H

#include "message/postoffice.hpp"

#include "network/connection_manager.hpp"
#include "network/stungun.hpp"

#include "security/security_library.hpp"

#include "util/thread.hpp"

#include <memory>
#include <map>

namespace fire
{
    namespace message
    {
        class master_post_office : public post_office
        {
            public:
                master_post_office(
                        const std::string& in_host,
                        network::port_type in_port,
                        security::encrypted_channels_ptr);
                virtual ~master_post_office();

            public:
                const network::udp_stats& get_udp_stats() const;

            protected:
                virtual bool send_outside(const message&);

            private:
                bool _stunned;
                std::string _in_host;
                network::port_type _in_port;
                network::stun_gun_ptr _stun;
                util::thread_uptr _in_thread;
                util::thread_uptr _out_thread;
                queue _out;
                network::connection_manager _connections;
                security::encrypted_channels_ptr _encrypted_channels;

            private:
                friend void in_thread(master_post_office* o);
                friend void out_thread(master_post_office* o);
        };

    }
}
#endif
