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
#ifndef FIRESTR_NETWORK_TCP_QUEUE_H
#define FIRESTR_NETWORK_TCP_QUEUE_H

#include "network/util.hpp"
#include "network/connection.hpp"
#include "network/message_queue.hpp"
#include "util/thread.hpp"

namespace fire
{
    namespace network
    {
        using tcp_resolver_ptr = std::unique_ptr<boost::asio::ip::tcp::resolver>;
        using tcp_acceptor_ptr = std::unique_ptr<boost::asio::ip::tcp::acceptor>;
        using tcp_socket_ptr = std::unique_ptr<boost::asio::ip::tcp::socket>;

        class tcp_connection;
        using tcp_connection_ptr_queue = util::queue<tcp_connection*>;
        using connection_ptr_queue = util::queue<connection*>;

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
                void bind(port_type port);
                void connect(boost::asio::ip::tcp::endpoint);
                void start_read();
                void close();
                bool is_connected() const;
                bool is_connecting() const;
                con_state state() const;
                boost::asio::ip::tcp::socket& socket();

            public:
                void update_endpoint();
                void update_endpoint(const std::string& address, port_type port);

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
                void connect(const std::string& host, port_type port);
                bool is_connected();
                bool is_connecting();

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

        tcp_queue_ptr create_tcp_queue(const address_components& c);
        tcp_queue_ptr create_tcp_queue(const std::string& address, const queue_options& defaults);
    }
}

#endif
