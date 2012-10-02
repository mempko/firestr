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
#ifndef FIRESTR_NETWORK_ZEROMQ_H
#define FIRESTR_NETWORK_ZEROMQ_H

#include "network/message_queue.hpp"
#include "util/bytes.hpp"

#include <memory>

#include <boost/cstdint.hpp>

#include <zmq.hpp>

namespace fire
{
    namespace network
    {

        typedef std::unique_ptr<zmq::context_t> context_ptr;
        typedef std::unique_ptr<zmq::socket_t> socket_ptr;

        enum socket_type {request, reply, push, pull, publish, subscribe};
        enum connect_mode {bind, connect}; 

        struct zmq_params
        {
            socket_type type;
            connect_mode mode;
            std::string uri;
            int threads;
            bool block;
            size_t timeout;
            double wait;
            int linger;
            boost::uint64_t hwm;
        };

        class zmq_queue : public message_queue
        {
            public:
                zmq_queue(const zmq_params& p);
                virtual ~zmq_queue();

            public:
                virtual bool send(const util::bytes& b);
                virtual bool recieve(util::bytes& b);

            public:
                virtual socket_info get_socket_info() const;

            private:
                bool timedout(short event);

            private:
                zmq_params _p;
                context_ptr _c;
                socket_ptr _s;
        };

        message_queue_ptr create_zmq_message_queue(const address_components& c);
    }
}

#endif
