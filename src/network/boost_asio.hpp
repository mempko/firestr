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
        typedef std::unique_ptr<boost::asio::ip::tcp::resolver> asio_resolver_ptr;
        typedef std::unique_ptr<boost::asio::ip::tcp::acceptor> asio_acceptor_ptr;
        typedef std::unique_ptr<boost::asio::ip::tcp::socket> asio_socket_ptr;

        class connection;
        typedef util::queue<connection*> connection_ptr_queue;

        class connection
        {
            public:
                enum con_state{connecting, connected, disconnected};

                connection(
                        boost::asio::io_service& io, 
                        byte_queue& in,
                        byte_queue& out,
                        connection_ptr_queue& last_in,
                        bool track = false,
                        bool con = false);
                ~connection();
            public:
                void connect(boost::asio::ip::tcp::resolver::iterator, const std::string&);
                void start_read();
                void close();
                bool is_connected() const;
                con_state state() const;
                boost::asio::ip::tcp::socket& socket();

            private:
                void handle_connect(const boost::system::error_code& error,
                        boost::asio::ip::tcp::resolver::iterator ei);
                void do_send(bool);
                void handle_write(const boost::system::error_code& error);
                void handle_header(const boost::system::error_code& error, size_t);
                void handle_body(const boost::system::error_code& error, size_t, size_t);
                void do_close();
            private:

                con_state _state;
                boost::asio::io_service& _io;
                byte_queue& _in_queue;
                byte_queue& _out_queue;
                connection_ptr_queue& _last_in_socket;
                bool _track;
                util::bytes _out_buffer;
                std::string _src_host;
                std::string _src_port;
                boost::asio::streambuf _in_buffer;
                asio_socket_ptr _socket;
                std::mutex _mutex;
                boost::system::error_code _error;
                bool _writing;
            private:
                friend class boost_asio_queue;
        };

        typedef std::shared_ptr<connection> connection_ptr;
        typedef std::vector<connection_ptr> connections;

        struct asio_params
        {
            enum connect_mode {bind, connect} mode; 
            std::string uri;
            std::string host;
            std::string port;
            std::string local_port;
            bool block;
            bool wait;
            bool track_incoming;
        };

        class boost_asio_queue : public message_queue
        {
            public:
                boost_asio_queue(const asio_params& p);
                virtual ~boost_asio_queue();

            public:
                virtual bool send(const util::bytes& b);
                virtual bool recieve(util::bytes& b);

            public:
                virtual socket_info get_socket_info() const;

            private:
                void connect();
                void accept();

            private:
                void handle_accept(connection_ptr nc, const boost::system::error_code& error);

            private:
                asio_params _p;
                asio_service_ptr _io;
                asio_resolver_ptr _resolver;
                asio_acceptor_ptr _acceptor;
                util::thread_uptr _run_thread;

                connection_ptr _out;
                mutable connection_ptr_queue _last_in_socket;
                connections _in_connections;
                byte_queue _in_queue;
                byte_queue _out_queue;

                bool _done;

            private:
                friend void run_thread(boost_asio_queue*);
        };

        message_queue_ptr create_bst_message_queue(const address_components& c);
    }
}

#endif
