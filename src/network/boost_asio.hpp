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
        typedef util::queue<tcp_connection*> tcp_connection_ptr_queue;

        class connection
        {
            public:
            virtual bool send(const fire::util::bytes& b, bool block) = 0;
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
                void bind(const std::string& port);
                void connect(boost::asio::ip::tcp::endpoint);
                void start_read();
                void close();
                bool is_connected() const;
                bool is_disconnected() const;
                bool is_connecting() const;
                con_state state() const;
                boost::asio::ip::tcp::socket& socket();

            public:
                const std::string& remote_address() const;
                void remote_address(const std::string&);
                void update_remote_address();

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
                std::string _src_host;
                std::string _src_port;
                std::string _remote_address;
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

        struct udp_endpoint
        {
            std::string addresss;
            std::string port;
        };

        typedef std::unique_ptr<boost::asio::ip::udp::resolver> udp_resolver_ptr;
        typedef std::unique_ptr<boost::asio::ip::udp::socket> udp_socket_ptr;

        class udp_connection : public connection
        {
            public:
                udp_connection(
                        const udp_endpoint& ep,
                        boost::asio::io_service& io, 
                        std::mutex& in_mutex);
            public:
                virtual bool send(const fire::util::bytes& b, bool block = false);

            private:
                byte_queue _out_queue;
                util::bytes _out_buffer;
                udp_socket_ptr _socket;
        };

        struct asio_params
        {
            enum connect_mode {bind, connect, delayed_connect} mode; 
            std::string uri;
            std::string host;
            std::string port;
            std::string local_port;
            bool block;
            double wait;
            bool track_incoming;
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
                tcp_connection* get_socket() const;
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
                friend void run_thread(tcp_queue*);
        };

        typedef std::shared_ptr<tcp_queue> tcp_queue_ptr;

        asio_params parse_params(const address_components& c);
        tcp_queue_ptr create_bst_message_queue(const address_components& c);
        std::string make_tcp_address(const std::string& host, const std::string& port, const std::string& local_port = "");
        tcp_queue_ptr create_message_queue(const std::string& address, const queue_options& defaults);

        socket_info get_socket_info(tcp_connection&);
    }
}

#endif
