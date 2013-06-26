/*
 * Copyright (C) 2013  Maxim Noah Khailo
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
#include <unordered_map>

#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/dynamic_bitset.hpp>

namespace fire
{
    namespace network
    {
        extern const std::string UDP;
        extern const std::string TCP;

        using byte_queue = util::queue<util::bytes>;
        using asio_service_ptr = std::unique_ptr<boost::asio::io_service>;
        using tcp_resolver_ptr = std::unique_ptr<boost::asio::ip::tcp::resolver>;
        using tcp_acceptor_ptr = std::unique_ptr<boost::asio::ip::tcp::acceptor>;
        using tcp_socket_ptr = std::unique_ptr<boost::asio::ip::tcp::socket>;

        class tcp_connection;
        class udp_connection;
        class connection;
        using tcp_connection_ptr_queue = util::queue<tcp_connection*>;
        using connection_ptr_queue = util::queue<connection*>;

        struct endpoint
        {
            std::string protocol;
            std::string address;
            std::string port;
        };

        struct endpoint_message
        {
            endpoint ep;
            util::bytes data;
        };

        using endpoint_queue = util::queue<endpoint_message>;

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

        using tcp_connection_ptr = std::shared_ptr<tcp_connection>;
        using tcp_connections = std::vector<tcp_connection_ptr>;

        using udp_resolver_ptr = std::unique_ptr<boost::asio::ip::udp::resolver>;
        using udp_socket_ptr = std::unique_ptr<boost::asio::ip::udp::socket>;
        using sequence_type = uint64_t;

        struct udp_chunk
        {
            bool valid;
            std::string host;
            std::string port;
            sequence_type sequence;
            int total_chunks;
            int chunk;
            util::bytes data;
        };

        using chunk_queue = util::queue<udp_chunk>;
        using udp_chunks = std::vector<udp_chunk>;

        struct working_udp_chunks
        {
            udp_chunks chunks;
            boost::dynamic_bitset<> set;
        };

        using working_udp_endpoints = std::unordered_map<sequence_type, working_udp_chunks>;
        using working_udp_messages = std::unordered_map<std::string, working_udp_endpoints>;

        class udp_connection
        {
            public:
                udp_connection(
                        endpoint_queue& in,
                        boost::asio::io_service& io, 
                        std::mutex& in_mutex);
            public:
                bool send(const endpoint_message& m, bool block = false);

            public:
                void bind(const std::string& port);
                void do_send(bool force);
                void handle_write(const boost::system::error_code& error);
                void handle_read(const boost::system::error_code& error, size_t transferred);
                void close();
                void start_read();
                void do_close();

            private:
                size_t chunkify(const std::string& host, const std::string& port, const fire::util::bytes& b);

            private:
                //reading
                util::bytes _in_buffer;
                boost::asio::ip::udp::endpoint _in_endpoint;
                working_udp_messages _in_working;
                std::mutex& _in_mutex;
                endpoint_queue& _in_queue;

                //writing
                chunk_queue _out_queue;
                util::bytes _out_buffer;

                //other
                boost::asio::io_service& _io;
                udp_socket_ptr _socket;
                uint64_t _sequence;
                bool _writing;
                mutable std::mutex _mutex;
                boost::system::error_code _error;
        };

        using udp_connection_ptr = std::shared_ptr<udp_connection>;
        using udp_connections = std::vector<udp_connection_ptr>;

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


        class udp_queue
        {
            public:
                udp_queue(const asio_params& p);
                virtual ~udp_queue();

            public:
                virtual bool send(const endpoint_message& m);
                virtual bool receive(endpoint_message& b);

            private:
                void bind();

            private:
                asio_params _p;
                asio_service_ptr _io;
                util::thread_uptr _run_thread;

                udp_connection_ptr _con;
                endpoint_queue _in_queue;
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
                virtual bool receive(util::bytes& b);

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

        using tcp_queue_ptr = std::shared_ptr<tcp_queue>;
        using udp_queue_ptr = std::shared_ptr<udp_queue>;

        asio_params parse_params(const address_components& c);
        asio_params::endpoint_type determine_type(const std::string& address);
        std::string make_tcp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        std::string make_udp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        tcp_queue_ptr create_tcp_queue(const address_components& c);
        tcp_queue_ptr create_tcp_queue(const std::string& address, const queue_options& defaults);
        udp_queue_ptr create_udp_queue(const asio_params& c);

        std::string make_address_str(const endpoint& e);
    }
}

#endif
