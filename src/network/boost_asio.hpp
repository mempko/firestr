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
#ifndef FIRESTR_NETWORK_BOOST_ASIO_H
#define FIRESTR_NETWORK_BOOST_ASIO_H

#include "network/message_queue.hpp"
#include "util/thread.hpp"
#include "util/queue.hpp"
#include "util/bytes.hpp"

#include <memory>

#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/dynamic_bitset.hpp>

namespace fire
{
    namespace network
    {
        typedef util::queue<util::bytes> byte_queue;
        typedef std::unique_ptr<boost::asio::io_service> asio_service_ptr;
        typedef std::unique_ptr<boost::asio::ip::tcp::resolver> tcp_resolver_ptr;
        typedef std::unique_ptr<boost::asio::ip::tcp::acceptor> tcp_acceptor_ptr;
        typedef std::unique_ptr<boost::asio::ip::tcp::socket> tcp_socket_ptr;

        class tcp_connection;
        class udp_connection;
        class connection;
        typedef util::queue<tcp_connection*> tcp_connection_ptr_queue;
        typedef util::queue<udp_connection*> udp_connection_ptr_queue;
        typedef util::queue<connection*> connection_ptr_queue;

        struct endpoint
        {
            std::string protocol;
            std::string address;
            std::string port;
        };

        class connection
        {
            public:
            virtual bool send(const fire::util::bytes& b, bool block = false) = 0;
            virtual endpoint get_endpoint() const = 0;
            virtual bool is_disconnected() const = 0;
        };

        class tcp_connection : public connection
        {
            public:
                enum con_state{connecting, connected, disconnected};

                tcp_connection(
                        boost::asio::io_service& io, 
                        byte_queue& in,
                        tcp_connection_ptr_queue& last_in,
                        std::mutex& in_mutex,
                        bool track = false,
                        bool con = false);
                ~tcp_connection();
            public:
                virtual bool send(const fire::util::bytes& b, bool block = false);
                virtual endpoint get_endpoint() const;
                virtual bool is_disconnected() const;

            public:
                void bind(const std::string& port);
                void connect(boost::asio::ip::tcp::endpoint);
                void start_read();
                void close();
                bool is_connected() const;
                bool is_connecting() const;
                con_state state() const;
                boost::asio::ip::tcp::socket& socket();

            public:
                void update_endpoint();
                void update_endpoint(const std::string& address, const std::string& port);

            private:
                void handle_connect(
                        const boost::system::error_code& error, 
                        boost::asio::ip::tcp::endpoint e);
                void handle_punch(const boost::system::error_code& error);
                void do_send(bool);
                void handle_write(const boost::system::error_code& error);
                void handle_header(const boost::system::error_code& error, size_t);
                void handle_body(const boost::system::error_code& error, size_t, size_t);
                void do_close();
            private:

                con_state _state;
                boost::asio::io_service& _io;
                byte_queue& _in_queue;
                std::mutex& _in_mutex;
                byte_queue _out_queue;
                tcp_connection_ptr_queue& _last_in_socket;
                bool _track;
                util::bytes _out_buffer;
                endpoint _ep;
                boost::asio::streambuf _in_buffer;
                tcp_socket_ptr _socket;
                mutable std::mutex _mutex;
                boost::system::error_code _error;
                bool _writing;
                int _retries;
            private:
                friend class tcp_queue;
        };

        typedef std::shared_ptr<tcp_connection> tcp_connection_ptr;
        typedef std::vector<tcp_connection_ptr> tcp_connections;

        typedef std::unique_ptr<boost::asio::ip::udp::resolver> udp_resolver_ptr;
        typedef std::unique_ptr<boost::asio::ip::udp::socket> udp_socket_ptr;
        typedef uint64_t sequence_type;

        struct udp_chunk
        {
            sequence_type sequence;
            int total_chunks;
            int chunk;
            util::bytes data;
        };
        typedef util::queue<udp_chunk> chunk_queue;
        typedef std::vector<udp_chunk> udp_chunks;

        //TODO: need to use endpoint as another indirection
        struct working_udp_chunks
        {
            udp_chunks chunks;
            boost::dynamic_bitset<> set;
        };

        typedef std::map<sequence_type, working_udp_chunks> working_udp_messages;

        class udp_connection : public connection
        {
            public:
                udp_connection(
                        const endpoint& ep,
                        byte_queue& in,
                        boost::asio::io_service& io, 
                        std::mutex& in_mutex);
            public:
                virtual bool send(const fire::util::bytes& b, bool block = false);
                virtual endpoint get_endpoint() const;
                virtual bool is_disconnected() const;

            public:
                void bind(const std::string& port);
                void do_send(bool force);
                void handle_write(const boost::system::error_code& error);
                void handle_read(const boost::system::error_code& error, size_t transferred);
                void close();

            private:
                void start_read();
                void chunkify(const fire::util::bytes& b);
                void do_close();

            private:
                //reading
                util::bytes _in_buffer;
                boost::asio::ip::udp::endpoint _in_endpoint;
                working_udp_messages _in_working;
                byte_queue& _in_queue;
                std::mutex& _in_mutex;

                //writing
                chunk_queue _out_queue;
                util::bytes _out_buffer;

                //other
                boost::asio::io_service& _io;
                udp_socket_ptr _socket;
                endpoint _ep;
                uint64_t _sequence;
                bool _writing;
                mutable std::mutex _mutex;
                boost::system::error_code _error;
        };

        typedef std::shared_ptr<udp_connection> udp_connection_ptr;
        typedef std::vector<udp_connection_ptr> udp_connections;

        struct asio_params
        {
            enum endpoint_type { tcp, udp} type;
            enum connect_mode {bind, connect, delayed_connect} mode; 
            std::string uri;
            std::string host;
            std::string port;
            std::string local_port;
            bool block;
            double wait;
            bool track_incoming;
        };


        typedef std::vector<util::bytes>  chunked_message;
        typedef std::map<size_t,chunked_message> incoming_message_buffers;
        typedef std::map<std::string, incoming_message_buffers> incoming_messages;

        class udp_queue : public message_queue
        {
            public:
                udp_queue(const asio_params& p);
                virtual ~udp_queue();

            public:
                virtual bool send(const util::bytes& b);
                virtual bool recieve(util::bytes& b);

            private:
                void connect();
                void accept();

            private:
                asio_params _p;
                asio_service_ptr _io;
                util::thread_uptr _run_thread;

                udp_connection_ptr _out;
                udp_connection_ptr _in;
                incoming_messages _incoming;
                byte_queue _in_queue;
                mutable std::mutex _mutex;
                bool _done;

            private:
                friend void udp_run_thread(udp_queue*);
        };

        class tcp_queue : public message_queue
        {
            public:
                tcp_queue(const asio_params& p);
                virtual ~tcp_queue();

            public:
                virtual bool send(const util::bytes& b);
                virtual bool recieve(util::bytes& b);

            public:
                connection* get_socket() const;
                void connect(const std::string& host, const std::string& port);

            private:
                void connect();
                void delayed_connect();
                void accept();

            private:
                void handle_accept(tcp_connection_ptr nc, const boost::system::error_code& error);

            private:
                asio_params _p;
                asio_service_ptr _io;
                tcp_resolver_ptr _resolver;
                tcp_acceptor_ptr _acceptor;
                util::thread_uptr _run_thread;

                tcp_connection_ptr _out;
                mutable tcp_connection_ptr_queue _last_in_socket;
                tcp_connections _in_connections;
                byte_queue _in_queue;
                mutable std::mutex _mutex;

                bool _done;

            private:
                friend void tcp_run_thread(tcp_queue*);
        };

        typedef std::shared_ptr<tcp_queue> tcp_queue_ptr;

        asio_params parse_params(const address_components& c);
        tcp_queue_ptr create_bst_message_queue(const address_components& c);
        std::string make_tcp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        tcp_queue_ptr create_message_queue(const std::string& address, const queue_options& defaults);

        std::string make_address_str(const endpoint& e);
    }
}

#endif
