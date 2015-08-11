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
#ifndef FIRESTR_NETWORK_UDP_QUEUE_H
#define FIRESTR_NETWORK_UDP_QUEUE_H

#include "network/util.hpp"
#include "network/connection.hpp"
#include "network/message_queue.hpp"
#include "util/thread.hpp"

#include <list>
#include <unordered_map>

namespace fire
{
    namespace network
    {
        struct endpoint_message
        {
            endpoint ep;
            util::bytes data;
            bool robust;
        };

        using endpoint_queue = util::queue<endpoint_message>;

        using udp_resolver_ptr = std::unique_ptr<boost::asio::ip::udp::resolver>;
        using udp_socket_ptr = std::unique_ptr<boost::asio::ip::udp::socket>;
        using sequence_type = uint64_t;
        using chunk_total_type = uint16_t;
        using chunk_id_type = uint16_t;

        struct message_chunk
        {
            bool valid = false;
            std::string host;
            port_type port = 0;
            sequence_type sequence = 0;
            chunk_total_type total_chunks = 0;
            chunk_id_type chunk;
            util::bytes data;
            bool resent = false;
            enum msg_type { qmsg, msg, ack} type;

            //used for writing
            const char* write_data = nullptr;
            size_t write_size = 0;
        };


        struct working_message
        {
            message_chunk proto;
            util::bytes data;
            boost::dynamic_bitset<> set;
            boost::dynamic_bitset<> sent;
            size_t ticks = 0;
            size_t in_flight = 0;
            size_t queued = 0;
            size_t next_send = 0;
        };

        //working set for both incoming and outgoing messages
        using hash_type = std::size_t;
        using working_messages = std::unordered_map<sequence_type, working_message>;
        using resolve_map = std::unordered_map<std::string, std::string>;

        //outgoing chunks are send round robin in the message_ring
        using chunk_id_queue = util::queue<chunk_id_type>;
        struct message_ring_item
        {
            working_message* wm;
            chunk_id_queue resends; 
        };

        using message_ring = std::vector<message_ring_item>;

        struct udp_stats
        {
            size_t dropped = 0;
            size_t bytes_sent = 0;
            size_t bytes_recv = 0;
        };

        using chunk_queue = util::queue<message_chunk>;

        class udp_queue;
        class udp_connection
        {
            public:
                udp_connection(
                        endpoint_queue& in,
                        boost::asio::io_service& io);
            public:
                bool send(const endpoint_message& m, bool block = false);

            public:
                void bind(port_type port);
                void do_send();
                void handle_write(const boost::system::error_code& error);
                void handle_read(const boost::system::error_code& error, size_t transferred);
                void close();
                void start_read();
                void do_close();
                const udp_stats& stats() const; 

            private:
                void add_to_working_set(endpoint_message m);
                void init_working(message_chunk& proto, util::bytes& data);
                void send_right_away(message_chunk& c);
                bool get_next_chunk(working_message&, message_chunk& queued_chunk);
                void cleanup_message(sequence_type sequence);
                void validate_chunk(const message_chunk& c);
                void queue_resend(message_ring_item&, chunk_id_type c);
                void queue_next_chunk();
                bool incr_next_message();
                void sent_chunk(const message_chunk& c);
                size_t resend(message_ring_item&);
                void resend();
                void post_send();

            private:
                //reading
                util::bytes _work_buffer;
                util::bytes _in_buffer;
                util::bytes _out_buffer;
                boost::asio::ip::udp::endpoint _in_endpoint;
                working_messages _in_working;
                working_messages _out_working;
                endpoint_queue& _in_queue;

                //writing
                size_t _next_message = 0;
                message_ring _message_ring; //messages get chunked to here

                //queue for chunks ready to go
                chunk_queue _out_queue; //the queue loop adds next message to here to be sent

                //other
                boost::asio::io_service& _io;
                udp_socket_ptr _socket;
                sequence_type _sequence = 0;
                bool _writing;
                boost::system::error_code _error;
                udp_stats _stats;
            private:
                friend void udp_run_thread(udp_queue*);
                friend void resend_thread(udp_queue*);
        };

        using udp_connection_ptr = std::shared_ptr<udp_connection>;
        using udp_connections = std::vector<udp_connection_ptr>;

        class udp_queue
        {
            public:
                udp_queue(const asio_params& p);
                virtual ~udp_queue();

            public:
                virtual bool send(const endpoint_message& m);
                virtual bool receive(endpoint_message& b);

            public:
                const udp_stats& stats() const; 

            private:
                void bind();
                const std::string& resolve(const endpoint&);

            private:
                asio_params _p;
                asio_service_ptr _io;
                util::thread_uptr _run_thread;
                util::thread_uptr _resend_thread;

                udp_connection_ptr _con;
                endpoint_queue _in_queue;
                udp_resolver_ptr _resolver;
                resolve_map _rmap;
                bool _done;

            private:
                friend void udp_run_thread(udp_queue*);
                friend void resend_thread(udp_queue*);
        };

        using udp_queue_ptr = std::shared_ptr<udp_queue>;

        udp_queue_ptr create_udp_queue(const asio_params& c);
    }
}

#endif
