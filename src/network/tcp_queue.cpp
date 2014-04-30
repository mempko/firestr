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
 */

#include "network/tcp_queue.hpp"
#include "util/thread.hpp"
#include "util/mencode.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <sstream>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

using boost::lexical_cast;
namespace u = fire::util;
namespace ba = boost::asio;
using namespace boost::asio::ip;

#ifdef __APPLE__
namespace so = boost::asio::detail::socket_option; 
using reuse_port = so::boolean<SOL_SOCKET, SO_REUSEPORT>;
#endif

namespace fire
{
    namespace network
    {
        namespace
        {
            const size_t BLOCK_SLEEP = 10;
            const size_t THREAD_SLEEP = 40;
            const size_t KEEP_ALIVE_SLEEP = 2000;
            const size_t KEEP_ALIVE_THRESH = 15;
            const u::bytes KEEP_ALIVE_MSG {'%', 'k'};
            const u::bytes KEEP_ALIVE_ACK_MSG {'%', 'a'};
            const int RETRIES = 0;
        }

        tcp_connection::tcp_connection(
                ba::io_service& io, 
                byte_queue& in,
                tcp_connection_ptr_queue& last_in,
                std::mutex& in_mutex,
                bool track,
                bool con) :
            _io(io),
            _in_queue(in),
            _in_mutex(in_mutex),
            _last_in_socket(last_in),
            _track{track},
            _socket{new tcp::socket{io}},
            _state{ con ? connected : disconnected},
            _writing{false},
            _retries{RETRIES}
        {
            INVARIANT(_socket);
        }

        tcp_connection::~tcp_connection()
        {
            close();
        }

        void tcp_connection::close()
        {
            _state = disconnected;
            _writing = false;
            _io.post(boost::bind(&tcp_connection::do_close, this));
        }

        void tcp_connection::do_close()
        {
            u::mutex_scoped_lock l(_mutex);
            _state = disconnected;
            _writing = false;
            if(_socket && _socket->is_open())
            {
                boost::system::error_code se;
                _socket->shutdown(ba::ip::tcp::socket::shutdown_both, se);
                _socket->close();
                if(se) _error = se;
                LOG << "tcp_connection closed " << _socket->local_endpoint() << " + " << _socket->remote_endpoint() << " error: " << _error.message() << std::endl;
            }
            else
            {
                LOG << "tcp_connection closed, socket already gone:  " << _ep.address << ":" << _ep.port << " error: " << _error.message() << std::endl;
            }
        }

        bool tcp_connection::is_connected() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _state == connected;
        }

        bool tcp_connection::is_disconnected() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _state == disconnected;
        }

        bool tcp_connection::is_connecting() const
        {
            u::mutex_scoped_lock l(_mutex);
            return _state == connecting;
        }

        tcp_connection::con_state tcp_connection::state() const
        {
            return _state;
        }

        void tcp_connection::bind(port_type port)
        {
            u::mutex_scoped_lock l(_mutex);
            INVARIANT(_socket);
            REQUIRE(_state == disconnected);

            boost::system::error_code error;

            _socket->open(tcp::v4(), error);
            _socket->set_option(tcp::socket::keep_alive(true),error);
            _socket->set_option(tcp::socket::reuse_address(true),error);
#ifdef __APPLE__
            _socket->set_option(reuse_port(true),error);
#endif
            auto p = boost::lexical_cast<short unsigned int>(port);
            _socket->bind(tcp::endpoint(tcp::v4(), p), error);

            if(error)
            {
                LOG << "error binding to port " << port << ": " << error.message() << std::endl;
                _error = error;
                _state = disconnected;
            }
        }

        void tcp_connection::connect(tcp::endpoint endpoint)
        {
            INVARIANT(_socket);

            {
                u::mutex_scoped_lock l(_mutex);
                if(_state != disconnected || !_socket->is_open()) return;
                _state = connecting;
            }

             LOG << "tcp connecting to " << _ep.address << ":" << _ep.port << " (" << endpoint << ")" <<std::endl;

            _socket->async_connect(endpoint,
                    boost::bind(&tcp_connection::handle_connect, this,
                        ba::placeholders::error, endpoint));
        }

        void tcp_connection::start_read()
        {
            INVARIANT(_socket);
            if(!_socket->is_open()) { close(); return; }

            //read message header
            ba::async_read_until(*_socket, _in_buffer, ':',
                    boost::bind(&tcp_connection::handle_header, this,
                        ba::placeholders::error,
                        ba::placeholders::bytes_transferred));
        }

        void tcp_connection::handle_connect(
                const boost::system::error_code& error,
                tcp::endpoint endpoint)
        {
            INVARIANT(_socket);
            if(_state != connecting) return;
            if(!_socket->is_open()) { close(); return; }

            //if no error then we are connected
            if (!error) 
            {
                {
                    u::mutex_scoped_lock l(_mutex);
                    _state = connected;
                }
                LOG << "new out tcp_connection " << _socket->local_endpoint() << " -> " << _socket->remote_endpoint() << ": " << error.message() << std::endl;
                start_read();

                //if we have called send already before we connected,
                //send the data
                if(!_out_queue.empty()) 
                    do_send(false);
            }
            else 
            {
                if(_retries > 0)
                {
                    LOG << "retrying (" << (RETRIES - _retries) << "/" << RETRIES << ")..." << std::endl;
                    {
                        {
                            u::mutex_scoped_lock l(_mutex);
                            _retries--;
                            _state = disconnected;
                        }
                        u::sleep_thread(2000);
                    }
                    connect(endpoint);
                }
                else
                {
                    {
                        u::mutex_scoped_lock l(_mutex);
                        _error = error;
                        LOG << "error connecting to `" << _ep.address << ":" << _ep.port << "' : " << error.message() << std::endl;
                    }
                    close();
                }
            }
        }

        u::bytes encode_tcp_wire(const u::bytes data)
        {
            auto m = u::encode(data);
            u::bytes encoded;
            encoded.resize(m.size() + 1);
            encoded[0] = '!';
            std::copy(m.begin(), m.end(), encoded.begin() + 1);
            return encoded;
        }

        bool tcp_connection::send(const u::bytes& b, bool block)
        {
            //add message to queue
            _out_queue.push(b);

            //do send if we are connected
            if(is_connected())
                _io.post(boost::bind(&tcp_connection::do_send, this, false));

            //if we are blocking, block until all messages are sent
            while(block && !_out_queue.empty()) u::sleep_thread(BLOCK_SLEEP);

            return is_connected();
        }

        void tcp_connection::send_keep_alive()
        {
            send(KEEP_ALIVE_MSG);
        }

        void tcp_connection::send_keep_alive_ack()
        {
            send(KEEP_ALIVE_ACK_MSG);
        }

        bool tcp_connection::is_alive() 
        {
            return _alive;
        }

        void tcp_connection::reset_alive() 
        {
            _alive = false;
        }

        void tcp_connection::do_send(bool force)
        {
            ENSURE(_socket);
            if(_state == disconnected) return;

            //check to see if a write is in progress
            if(!force && _writing) return;

            REQUIRE_FALSE(_out_queue.empty());
            _writing = true;

            //encode bytes to wire format
            _out_buffer = encode_tcp_wire(_out_queue.front());

            ENSURE(_writing);
            ba::async_write(*_socket,
                    ba::buffer(&_out_buffer[0], _out_buffer.size()),
                        boost::bind(&tcp_connection::handle_write, this,
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred));
        }

        void tcp_connection::handle_write(const boost::system::error_code& error, size_t transferred)
        {
            if(_state == disconnected) { CHECK_FALSE(_writing); return;}
            if(error) { _error = error; close(); return; }
            if(!_socket->is_open()) { close(); return; }

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
            ENSURE(_writing);
        }

        void tcp_connection::handle_header(const boost::system::error_code& error, size_t transferred)
        {
            INVARIANT(_socket);
            if(_state == disconnected) return;
            if(error) { _error = error; close(); return; }
            if(!_socket->is_open()) { close(); return; }

            //read header 
            size_t o_size = _in_buffer.size();
            std::istream in(&_in_buffer);
            std::string size_buf;
            size_buf.reserve(64);
            int c = in.get();
            size_t garbage = 1;

            //find start of message
            while(c != '!' && in.good()) 
            { 
                c = in.get(); 
                garbage++;
            }
            if(!in.good()) { start_read(); return;}

            //read size
            c = in.get();
            size_t rc = 1;
            while(c != ':' && in.good())
            {
                size_buf.push_back(c);
                c = in.get();
                rc++;
            }
            if(!in.good()) { start_read(); return;}

            CHECK_EQUAL(_in_buffer.size(), o_size - rc - garbage);

            //otherwise we got a generic message and need to read the body.
            size_t size = 0; 
            try { size = lexical_cast<size_t>(size_buf); } catch (...){}

            if(size == 0) 
            {
                start_read();
                return;
            }

            if(_in_buffer.size() >= size) 
                handle_body(boost::system::error_code(), 0, size);
            else
            {
                //read body
                ba::async_read(*_socket,
                        _in_buffer,
                        ba::transfer_at_least(size - _in_buffer.size()),
                        boost::bind(&tcp_connection::handle_body, this,
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred,
                            size));
            }
        }

        void tcp_connection::handle_body(const boost::system::error_code& error, size_t transferred, size_t size)
        {
            INVARIANT(_socket);
            REQUIRE_GREATER(size, 0);
            if(_state == disconnected) return;
            if(error) { _error = error; close(); return; }
            if(!_socket->is_open()) { close(); return; }

            if(_in_buffer.size() < size) 
            {
                ba::async_read(*_socket,
                        _in_buffer,
                        ba::transfer_at_least(size - _in_buffer.size()),
                        boost::bind(&tcp_connection::handle_body, this,
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred,
                            size));
                return;
            }

            u::bytes data;
            data.resize(size);
            std::istream in(&_in_buffer);
            in.read(&data[0], size);

            //got keepalive or ack
            if(data == KEEP_ALIVE_MSG) send_keep_alive_ack();
            else if (data == KEEP_ALIVE_ACK_MSG) _alive = true;
            //otherwise add message to in queue
            else
            {
                u::mutex_scoped_lock l(_in_mutex);
                _in_queue.emplace_push(data);
                if(_track) _last_in_socket.push(this);
            }

            //read next message
            start_read();
        }

        tcp::socket& tcp_connection::socket()
        {
            INVARIANT(_socket);
            return *_socket;
        }

        endpoint tcp_connection::get_endpoint() const
        {
            return _ep;
        }

        void tcp_connection::update_endpoint(const std::string& address, port_type port)
        {
            REQUIRE_FALSE(address.empty());
            REQUIRE_GREATER(port, 0);

            _ep.protocol = TCP;
            _ep.address = address;
            _ep.port = port;

            ENSURE_FALSE(_ep.address.empty());
            ENSURE_GREATER(_ep.port, 0);
        }

        void tcp_connection::update_endpoint()
        {
            INVARIANT(_socket);

            auto remote = _socket->remote_endpoint();
            _ep.protocol = TCP;
            _ep.address = remote.address().to_string();
            _ep.port = remote.port();

            ENSURE_FALSE(_ep.address.empty());
            ENSURE_GREATER(_ep.port, 0);
        }

        void tcp_run_thread(tcp_queue*);
        void keep_alive_thread(tcp_queue*);
        tcp_queue::tcp_queue(const asio_params& p) : 
            _p(p), _done{false},
            _io{new ba::io_service}
        {
            switch(_p.mode)
            {
                case asio_params::bind: accept(); break;
                case asio_params::connect: connect(); break;
                case asio_params::delayed_connect: delayed_connect(); break;
                default: CHECK(false && "missed case");
            }

            if(_p.mode != asio_params::delayed_connect)
            {
                _run_thread.reset(new std::thread{tcp_run_thread, this});
                _keep_alive_thread.reset(new std::thread{keep_alive_thread, this});
            }

            INVARIANT(_io);
            INVARIANT(_p.mode == asio_params::delayed_connect || (_run_thread && _keep_alive_thread));
        }

        tcp_queue::~tcp_queue() 
        {
            INVARIANT(_io);
            if(_out) _out->close();

            _done = true;
            _io->stop();
            if(_p.block) _in_queue.done();
            if(_p.wait > 0) u::sleep_thread(_p.wait);
            if(_run_thread) _run_thread->join();
            if(_keep_alive_thread) _keep_alive_thread->join();
        }

        bool tcp_queue::send(const u::bytes& b)
        {
            INVARIANT(_io);
            REQUIRE(_p.mode != asio_params::bind);
            CHECK(_out);

            if(_out->is_disconnected() && _p.mode == asio_params::connect) 
                connect();

            return _out->send(b, _p.block);
        }

        bool tcp_queue::receive(u::bytes& b)
        {
            //return true if we got message
            return _in_queue.pop(b, _p.block);
        }

        void tcp_queue::connect(const std::string& host, port_type port)
        {
            REQUIRE(_p.mode == asio_params::delayed_connect);
            REQUIRE_FALSE(host.empty());
            REQUIRE_FALSE(_run_thread);
            INVARIANT(_io);
            REQUIRE(!_out || _out->state() == tcp_connection::disconnected);

            _p.uri = make_tcp_address(host, port);
            _p.host = host;
            _p.port = port;
            _p.mode = asio_params::connect;
            connect();

            //start up engine
            _run_thread.reset(new std::thread{tcp_run_thread, this});
            _keep_alive_thread.reset(new std::thread{keep_alive_thread, this});
            ENSURE(_run_thread);
            ENSURE(_keep_alive_thread);
        }

        bool tcp_queue::is_connected()
        {
            return _out && _out->is_connected();
        }

        bool tcp_queue::is_connecting()
        {
            return _out && _out->is_connecting();
        }

        bool tcp_queue::is_disconnected()
        {
            return _out && _out->is_disconnected();
        }

        void tcp_queue::delayed_connect()
        try
        {
            INVARIANT(_io);
            REQUIRE(!_out);

            _out.reset(new tcp_connection{*_io, _in_queue, _last_in_socket, _mutex});
            if(_p.local_port > 0) _out->bind(_p.local_port);

            ENSURE(_out);
        }
        catch(std::exception& e)
        {
            LOG << "error in delayed connecting `" << _p.local_port << "' : " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "error in delayed connecting `" << _p.local_port << "' : unknown error." << std::endl;
        }

        void tcp_queue::connect()
        try
        {
            INVARIANT(_io);
            REQUIRE(!_out || _out->state() == tcp_connection::disconnected);

            //init resolver if it does not exist
            if(!_resolver) _resolver.reset(new tcp::resolver{*_io}); 

            tcp::resolver::query query{_p.host, port_to_string(_p.port)}; 
            auto ei = _resolver->resolve(query);
            auto endpoint = *ei;

            if(!_out) delayed_connect();

            _out->update_endpoint(_p.host, _p.port);
            _out->connect(endpoint);

            ENSURE(_out);
            ENSURE(_resolver);
        }
        catch(std::exception& e)
        {
            LOG << "error connecting to `" << _p.host << ":" << _p.port << "' : " << e.what() << std::endl;
        }
        catch(...)
        {
            LOG << "error connecting to `" << _p.host << ":" << _p.port << "' : unknown error." << std::endl;
        }

        void tcp_queue::accept()
        {
            INVARIANT(_io);

            if(!_acceptor) 
            {
                _acceptor.reset(new tcp::acceptor{*_io});

                auto port = boost::lexical_cast<short unsigned int>(_p.port);
                tcp::endpoint endpoint{tcp::v4(), port}; 

                _acceptor->open(endpoint.protocol());
                _acceptor->set_option(tcp::acceptor::keep_alive(true));
                _acceptor->set_option(tcp::acceptor::reuse_address(true));
#ifdef __APPLE__
                _acceptor->set_option(reuse_port(true));
#endif
                _acceptor->bind(endpoint);
                _acceptor->listen();
            }

            //prepare incoming tcp_connection
            tcp_connection_ptr new_connection{new tcp_connection{*_io, _in_queue, _last_in_socket, _mutex, _p.track_incoming, true}};
            _acceptor->async_accept(new_connection->socket(),
                    bind(&tcp_queue::handle_accept, this, new_connection,
                        ba::placeholders::error));

            ENSURE(_acceptor);
        }

        void tcp_queue::handle_accept(tcp_connection_ptr nc, const boost::system::error_code& error)
        {
            REQUIRE(nc);
            INVARIANT(_acceptor);

            if(error) 
            {
                nc->_state = tcp_connection::disconnected;
                LOG << "error accept " << nc->socket().local_endpoint() << " -> " << nc->socket().remote_endpoint() << ": " << error.message() << std::endl;
                return;
            }
            LOG << "new in tcp_connection " << nc->socket().remote_endpoint() << " " << error.message() << std::endl;

            _in_connections.push_back(nc);
            nc->update_endpoint();
            nc->start_read();

            //prepare next incoming tcp_connection
            tcp_connection_ptr new_connection{new tcp_connection{*_io, _in_queue, _last_in_socket, _mutex, _p.track_incoming, true}};
            _acceptor->async_accept(new_connection->socket(),
                    boost::bind(&tcp_queue::handle_accept, this, new_connection,
                        ba::placeholders::error));

            ENSURE(new_connection);
        }

        connection* tcp_queue::get_socket() const
        {
            tcp_connection* p = nullptr;
            switch(_p.mode)
            {
                case asio_params::bind: if(_p.track_incoming) _last_in_socket.pop(p); break;
                case asio_params::delayed_connect:
                case asio_params::connect: p = _out.get(); break;
                default:
                    CHECK(false && "missed case");
            }
            return p;
        }

        void tcp_run_thread(tcp_queue* q)
        {
            CHECK(q);
            CHECK(q->_io);
            size_t ticks = 0;
            if(q->_out) q->_out->send_keep_alive();

            while(!q->_done) 
            try
            {
                q->_io->run();
                u::sleep_thread(THREAD_SLEEP);
                if(q->_out && q->_out->is_disconnected())
                    break;
            }
            catch(std::exception& e)
            {
                LOG << "error in tcp thread. " << e.what() << std::endl;
                if(q->_out) q->_out->close();
            }
            catch(...)
            {
                LOG << "unknown error in tcp thread." << std::endl;
                if(q->_out) q->_out->close();
            }
        }

        void keep_alive_thread(tcp_queue* q)
        {
            CHECK(q);
            size_t ticks = 0;
            if(q->_out) q->_out->send_keep_alive();

            while(!q->_done) 
            try
            {
                //do keepalive
                if(q->_out && !q->_out->is_disconnected() && ticks > KEEP_ALIVE_THRESH)
                {
                    if(q->_out->is_alive()) 
                    {
                        q->_out->send_keep_alive();
                        q->_out->reset_alive();
                    }
                    else q->_out->close();
                    ticks = 0;
                }
                ticks++;
                u::sleep_thread(KEEP_ALIVE_SLEEP);

                if(q->_out && q->_out->is_disconnected())
                    break;
            }
            catch(std::exception& e)
            {
                LOG << "error in tcp thread. " << e.what() << std::endl;
                if(q->_out) q->_out->close();
            }
            catch(...)
            {
                LOG << "unknown error in tcp thread." << std::endl;
                if(q->_out) q->_out->close();
            }
        }

        tcp_queue_ptr create_tcp_queue(const address_components& c)
        {
            auto p = parse_params(c);
            return tcp_queue_ptr{new tcp_queue{p}};
        }

        tcp_queue_ptr create_tcp_queue(
                const std::string& address, 
                const queue_options& defaults)
        {
            auto c = parse_address(address, defaults); 
            tcp_queue_ptr p = create_tcp_queue(c);
            ENSURE(p);
            return p;
        }
    }
}
