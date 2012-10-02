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

#include "network/boost_asio.hpp"
#include "util/thread.hpp"
#include "util/mencode.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"

#include <stdexcept>
#include <sstream>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

using boost::lexical_cast;
namespace u = fire::util;
namespace ba = boost::asio;
using namespace boost::asio::ip;
using boost::bind;

namespace fire
{
    namespace network
    {
        namespace
        {
            const size_t BLOCK_SLEEP = 10;

            asio_params::connect_mode determine_connection_mode(const queue_options& o)
            {
                asio_params::connect_mode m = asio_params::connect;

                if(o.count("bnd")) m = asio_params::bind;
                else if(o.count("con")) m = asio_params::connect;

                return m;
            }

            typedef std::vector<std::string> strings;
            void parse_uri(const std::string& uri, std::string& host, std::string& port)
            {
                //e.g. tcp://<host>:<port>
                auto s = u::split<strings>(uri, ":");
                if(s.size() != 3) return;
                if(s[1].size() <= 2) return;
                //should start with "//"
                host = s[1].substr(2, s[1].size() - 2);
                port = s[2];
            }

            asio_params parse_params(const address_components& c)
            {
                const auto& o = c.options;

                asio_params p;
                p.mode = determine_connection_mode(o);
                p.uri = c.location;
                parse_uri(p.uri, p.host, p.port);
                p.local_port = get_opt(o, "local_port", std::string(""));
                p.block = get_opt(o, "block", 0);
                p.wait = get_opt(o, "wait", 0);
                p.track_incoming = get_opt(o, "track_incoming", 0);

                return p;
            }
        }

        connection::connection(
                ba::io_service& io, 
                byte_queue& in,
                byte_queue& out,
                connection_ptr_queue& last_in,
                bool track,
                bool con) :
            _io(io),
            _in_queue(in),
            _out_queue(out),
            _last_in_socket(last_in),
            _track{track},
            _socket{new tcp::socket{io}},
            _state{ con ? connected : disconnected},
            _writing{false}
        {
            INVARIANT(_socket);
        }

        connection::~connection()
        {
            close();
        }

        void connection::close()
        {
            _io.post(bind(&connection::do_close, this));
        }

        void connection::do_close()
        {
            INVARIANT(_socket);
            u::mutex_scoped_lock l(_mutex);
            std::cerr << "connection closed " << _socket->local_endpoint() << " + " << _socket->remote_endpoint() << " error: " << _error.message() << std::endl;
            _socket->close();
            _state = disconnected;
            _writing = false;
        }

        bool connection::is_connected() const
        {
            return _state == connected;
        }

        connection::con_state connection::state() const
        {
            return _state;
        }

        void connection::connect(tcp::resolver::iterator e, const std::string& local_port)
        {
            INVARIANT(_socket);

            _state = connecting;
            auto endpoint = *e;
            if(!local_port.empty())
            {
                boost::system::error_code error;
                _socket->open(tcp::v4(), error);
                _socket->set_option(tcp::socket::reuse_address(true),error);
                auto port = boost::lexical_cast<short unsigned int>(local_port);
                _socket->bind(tcp::endpoint(tcp::v4(), port), error);
                if(error)
                {
                    _state = disconnected;
                    return;
                }
            } 

            _socket->async_connect(endpoint,
                    bind(&connection::handle_connect, this,
                        ba::placeholders::error, ++e));
        }

        void connection::start_read()
        {
            INVARIANT(_socket);

            //read message header
            ba::async_read_until(*_socket, _in_buffer, ':',
                    bind(&connection::handle_header, this,
                        ba::placeholders::error,
                        ba::placeholders::bytes_transferred));
        }

        void connection::handle_connect(
                const boost::system::error_code& error,
                tcp::resolver::iterator e)
        {
            INVARIANT(_socket);
            ENSURE(_state == connecting);

            //if no error then we are connected
            if (!error) 
            {
                u::mutex_scoped_lock l(_mutex);
                _state = connected;
                std::cerr << "new out connection " << _socket->local_endpoint() << " -> " << _socket->remote_endpoint() << ": " << error.message() << std::endl;
                start_read();

                //if we have called send already before we connected,
                //send the data
                if(!_out_queue.empty()) 
                    do_send(false);
            }
            //otherwise try next endpoint in list
            else if (e != tcp::resolver::iterator())
            {
                _error = error;
                auto endpoint = *e;
                _socket->async_connect(endpoint,
                        bind(&connection::handle_connect, this,
                            ba::placeholders::error, ++e));
            }
            else 
            {
                _error = error;
                _state = disconnected;
            }
            if(error)
            {
                std::cerr << "error connecting: " << error.message() << std::endl;
            }
        }

        u::bytes encode_wire(const u::bytes data)
        {
            auto m = u::encode(data);
            u::bytes encoded;
            encoded.resize(m.size() + 1);
            encoded[0] = '!';
            std::copy(m.begin(), m.end(), encoded.begin() + 1);
            return encoded;
        }

        void connection::do_send(bool force)
        {
            REQUIRE_FALSE(_out_queue.empty());
            ENSURE(_socket);

            //check to see if a write is in progress
            if(!force && _writing) return;

            _writing = true;

            //encode bytes to wire format
            _out_buffer = encode_wire(_out_queue.front());

            ba::async_write(*_socket,
                    ba::buffer(&_out_buffer[0], _out_buffer.size()),
                        bind(&connection::handle_write, this,
                            ba::placeholders::error));
        }

        void connection::handle_write(const boost::system::error_code& error)
        {
            if(error) { _error = error; close(); return; }

            INVARIANT(_socket);
            REQUIRE_FALSE(_out_queue.empty());

            //remove sent message
            _out_queue.pop_front();

            //if we are done sending finish the async write chain
            if(_out_queue.empty()) 
            {
                _writing = false;
                return;
            }

            //otherwise do another async write
            do_send(true);
        }

        void connection::handle_header(const boost::system::error_code& error, size_t transferred)
        {
            INVARIANT(_socket);

            if(error) { _error = error; close(); return; }

            //read header 
            size_t o_size = _in_buffer.size();
            std::istream in(&_in_buffer);
            std::string size_buf;
            size_buf.reserve(64);
            int c = in.get();
            size_t garbage = 1;

            while(c != '!' && in.good()) 
            { 
                c = in.get(); 
                garbage++;
            }

            c = in.get();
            size_t rc = 1;
            while(c != ':' && in.good())
            {
                size_buf.push_back(c);
                c = in.get();
                rc++;
            }

            CHECK_EQUAL(_in_buffer.size(), o_size - rc - garbage);

            size_t size = 0; 
            try { size = lexical_cast<size_t>(size_buf); } catch (...){}

            if(size == 0) 
            {
                start_read();
                return;
            }

            //read body
            ba::async_read(*_socket,
                    _in_buffer,
                    ba::transfer_at_least(size - _in_buffer.size()),
                    bind(&connection::handle_body, this,
                        ba::placeholders::error,
                        ba::placeholders::bytes_transferred,
                        size));
        }

        void connection::handle_body(const boost::system::error_code& error, size_t transferred, size_t size)
        {
            if(error && error == ba::error::eof) { _error = error; close(); return; }
            if(_in_buffer.size() < size) { _error = error; close(); return;}

            u::bytes data;
            data.resize(size);
            std::istream in(&_in_buffer);
            in.read(&data[0], size);

            //add message to in queue
            _in_queue.push(data);
            if(_track) _last_in_socket.push(this);

            //read next message
            start_read();
        }

        tcp::socket& connection::socket()
        {
            INVARIANT(_socket);
            return *_socket;
        }

        void run_thread(boost_asio_queue*);
        boost_asio_queue::boost_asio_queue(const asio_params& p) : 
            _p(p), _done{false}
        {
            _io.reset(new ba::io_service);

            switch(_p.mode)
            {
                case asio_params::bind: accept(); break;
                case asio_params::connect: connect(); break;
                default: CHECK(false && "missed case");
            }

            _run_thread.reset(new std::thread{run_thread, this});

            INVARIANT(_io);
            INVARIANT(_run_thread);
        }

        boost_asio_queue::~boost_asio_queue() 
        {
            INVARIANT(_run_thread);
            _done = true;

            _run_thread->join();
        }

        bool boost_asio_queue::send(const u::bytes& b)
        {
            INVARIANT(_io);
            if(_p.mode != asio_params::connect) return false;

            if(!_out) return false;
            CHECK(_out);

            if(_out->state() == connection::disconnected) 
                connect();

            //add message to queue
            _out_queue.push(b);

            //do send if we are connected
            if(_out->state() == connection::connected)
                _io->post(bind(&connection::do_send, _out.get(), false));

            //if we are blocking, block until all messages are sent
            while(_p.block && !_out_queue.empty()) u::sleep_thread(BLOCK_SLEEP);

            return _out->is_connected();
        }

        bool boost_asio_queue::recieve(u::bytes& b)
        {
            //if we are blocking, block until we get message
            while(_p.block && !_in_queue.empty()) u::sleep_thread(BLOCK_SLEEP);

            //return true if we got message
            return _in_queue.pop(b);
        }

        void boost_asio_queue::connect()
        try
        {
            INVARIANT(_io);
            if(_out && _out->state() != connection::disconnected) return;

            //init resolver if it does not exist
            if(!_resolver) _resolver.reset(new tcp::resolver{*_io}); 

            tcp::resolver::query query{_p.host, _p.port}; 
            auto ei = _resolver->resolve(query);

            _out.reset(new connection{*_io, _in_queue, _out_queue, _last_in_socket});
            _out->connect(ei, _p.local_port);

            ENSURE(_resolver);
            ENSURE(_out);
        }
        catch(std::exception& e)
        {
            std::cerr << "error connecting to `" << _p.host << ":" << _p.port << "' : " << e.what() << std::endl;
        }
        catch(...)
        {
            std::cerr << "error connecting to `" << _p.host << ":" << _p.port << "' : unknown error." << std::endl;
        }

        void boost_asio_queue::accept()
        {
            INVARIANT(_io);

            if(!_acceptor) 
            {
                _acceptor.reset(new tcp::acceptor{*_io});

                auto port = boost::lexical_cast<short unsigned int>(_p.port);
                tcp::endpoint endpoint{tcp::v4(), port}; 

                _acceptor->open(endpoint.protocol());
                _acceptor->set_option(tcp::acceptor::reuse_address(true));
                _acceptor->bind(endpoint);
                _acceptor->listen();
            }

            //prepare incoming connection
            connection_ptr new_connection{new connection{*_io, _in_queue, _out_queue, _last_in_socket, _p.track_incoming, true}};
            _acceptor->async_accept(new_connection->socket(),
                    bind(&boost_asio_queue::handle_accept, this, new_connection,
                        ba::placeholders::error));

            ENSURE(_acceptor);
        }

        void boost_asio_queue::handle_accept(connection_ptr nc, const boost::system::error_code& error)
        {
            REQUIRE(nc);
            INVARIANT(_acceptor);

            if(error) 
            {
                std::cerr << "error accept " << nc->socket().local_endpoint() << " -> " << nc->socket().remote_endpoint() << ": " << error.message() << std::endl;
                return;
            }
            std::cerr << "new in connection " << nc->socket().remote_endpoint() << ": " << error.message() << std::endl;

            _in_connections.push_back(nc);
            nc->start_read();

            //prepare next incoming connection
            connection_ptr new_connection{new connection{*_io, _in_queue, _out_queue, _last_in_socket, _p.track_incoming, true}};
            _acceptor->async_accept(new_connection->socket(),
                    bind(&boost_asio_queue::handle_accept, this, new_connection,
                        ba::placeholders::error));

            ENSURE(new_connection);
        }

        socket_info boost_asio_queue::get_socket_info() const
        {
            socket_info r;
            switch(_p.mode)
            {
                case asio_params::bind: 
                    {
                        INVARIANT(_acceptor);
                        auto local = _acceptor->local_endpoint();
                        r.local_address = local.address().to_string();
                        r.local_port = boost::lexical_cast<std::string>(local.port());
                        if(_p.track_incoming)
                        {
                            connection* i = 0;
                            _last_in_socket.pop(i);
                            if(i)
                            {
                                auto remote = i->socket().remote_endpoint();
                                r.remote_address = remote.address().to_string();
                                r.remote_port = boost::lexical_cast<std::string>(remote.port());
                            }
                        }
                    }
                    break;
                case asio_params::connect: 
                    {
                        INVARIANT(_out)
                        if(_out->is_connected())
                        {
                            auto local = _out->socket().local_endpoint();
                            auto remote = _out->socket().remote_endpoint();
                            r.local_address = local.address().to_string();
                            r.local_port = boost::lexical_cast<std::string>(local.port());
                            r.remote_address = remote.address().to_string();
                            r.remote_port = boost::lexical_cast<std::string>(remote.port());
                        }
                    }
                    break;
                default:
                    CHECK(false && "missed case");
            }

            return r;
        }

        message_queue_ptr create_bst_message_queue(const address_components& c)
        {
            auto p = parse_params(c);
            return message_queue_ptr{new boost_asio_queue{p}};
        }

        void run_thread(boost_asio_queue* q)
        {
            CHECK(q);
            CHECK(q->_io);
            while(!q->_done) 
            {
                q->_io->run_one();
                u::sleep_thread(BLOCK_SLEEP);
            }
        }
    }
}
