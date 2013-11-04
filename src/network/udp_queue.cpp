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

#include "network/udp_queue.hpp"
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

namespace so = boost::asio::detail::socket_option; 
using time_to_live = so::integer<IPPROTO_IP, IP_TTL>; 
#ifdef __APPLE__
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
            const int RETRIES = 0;
            const size_t MAX_UDP_BUFF_SIZE = 2048; //in bytes
            const size_t SEQUENCE_BASE = 1;
            const size_t CHUNK_TOTAL_BASE = SEQUENCE_BASE + sizeof(sequence_type);
            const size_t CHUNK_BASE = CHUNK_TOTAL_BASE + sizeof(chunk_total_type);
            const size_t MESSAGE_BASE = CHUNK_BASE + sizeof(chunk_id_type);

            //<mark> <sequence num> <chunk total> <chunk>
            const size_t HEADER_SIZE = MESSAGE_BASE;
            const size_t UDP_CHuNK_SIZE = 1024-HEADER_SIZE; //in bytes
            const size_t MAX_MESSAGE_SIZE = std::pow(2,sizeof(chunk_total_type)*8) * UDP_CHuNK_SIZE;
        }

        udp_queue_ptr create_udp_queue(const asio_params& p)
        {
            return udp_queue_ptr{new udp_queue{p}};
        }

        //plan
        //when recieving data on a udp socket
        //each packet will have: 
        //      1. message sequence number, 
        //      2. chunk sequence number 
        //      3. number of chunks in message
        //
        //      we keep track of messages by incoming port and address in the 
        //      incoming_messages map.
        //
        //      which has a map of messages being constructed.
        //      
        //      when a chunk is recieved, it is put in the poper slot in the 
        //      buffer.
        //
        //      if a message is complete, it is added to the in_queue and removed
        //      from the buffers.
        //
        //      I will have to come up with a way to remove stale buffers.
        //
        //      This way out of order udp packets will be ordered
        //      and we will also know which messages did not get completed.
        //      maybe in the future i'll send ACK type messages for all chunks recieved
        //      and do something like TCP over UDP.
        //
        // Sending a message is simpler
        //
        //  each message is split by chunks (a bit smaller than udp packet limit)
        //  and send across the network as these chunks. 
        //
        //
        udp_connection::udp_connection(
                endpoint_queue& in,
                boost::asio::io_service& io, 
                std::mutex& in_mutex) :
            _in_queue(in),
            _socket{new udp::socket{io}},
            _in_mutex(in_mutex),
            _io(io),
            _writing{false},
            _in_buffer(MAX_UDP_BUFF_SIZE)
        {
            boost::system::error_code error;
            _socket->open(udp::v4(), error);

            INVARIANT(_socket);
        }

        void udp_connection::close()
        {
            _io.post(boost::bind(&udp_connection::do_close, this));
        }

        void udp_connection::do_close()
        {
            INVARIANT(_socket);
            u::mutex_scoped_lock l(_mutex);
            _socket->close();
            _writing = false;
        }

        void udp_connection::send(udp_chunk& c)
        {
            //push chunk to queue
            _out_queue.emplace_push(c);
        }

        size_t udp_connection::chunkify(
                const std::string& host, 
                const std::string& port, 
                const fire::util::bytes& b)
        {
            REQUIRE_FALSE(b.empty());

            int total_chunks = b.size() < UDP_CHuNK_SIZE ? 
                1 : (b.size() / UDP_CHuNK_SIZE) + 1;

            CHECK_GREATER(total_chunks, 0);

            int chunk = 0;
            size_t s = 0;
            size_t e = std::min(b.size(), UDP_CHuNK_SIZE);

            CHECK_GREATER(total_chunks, 0);

            while(s < b.size())
            {
                size_t size = e - s;
                CHECK_GREATER(size, 0);

                //create chunk
                udp_chunk c;
                c.type = udp_chunk::msg;
                c.host = host;
                c.port = port;
                c.sequence = _sequence;
                c.total_chunks = total_chunks;
                c.chunk = chunk;
                c.data.resize(size);
                std::copy(b.begin() + s, b.begin() + e, c.data.begin());

                //push to out queue
                send(c);

                //step
                chunk++;
                s = e;
                e+=UDP_CHuNK_SIZE;
                if(e > b.size()) e = b.size();
            }

            CHECK_EQUAL(chunk, total_chunks);
            return chunk;

        }

        bool udp_connection::send(const endpoint_message& m, bool block)
        {
            INVARIANT(_socket);
            if(m.data.empty()) return false;
            if(m.data.size() > MAX_MESSAGE_SIZE)
            {
                LOG << "message of size `" << m.data.size() << "' is larger than the max message size of `" << MAX_MESSAGE_SIZE << "'" << std::endl;
                return false;
            }

            _sequence++;
            size_t chunks = chunkify(m.ep.address, m.ep.port, m.data);

            //post to do send
            _io.post(boost::bind(&udp_connection::do_send, this, false));

            //if we are blocking, block until all messages are sent
            while(block && !_out_queue.empty()) u::sleep_thread(BLOCK_SLEEP);

            return true;
        }

        void write_be(u::bytes& b, size_t offset, uint64_t v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint64_t));

            b[offset]     = (v >> 56) & 0xFF;
            b[offset + 1] = (v >> 48) & 0xFF;
            b[offset + 2] = (v >> 40) & 0xFF;
            b[offset + 3] = (v >> 32) & 0xFF;
            b[offset + 4] = (v >> 24) & 0xFF;
            b[offset + 5] = (v >> 16) & 0xFF;
            b[offset + 6] = (v >>  8) & 0xFF;
            b[offset + 7] =  v        & 0xFF;
        }

        void write_be(u::bytes& b, size_t offset, int v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(int));

            b[offset]     = (v >> 24) & 0xFF;
            b[offset + 1] = (v >> 16) & 0xFF;
            b[offset + 2] = (v >>  8) & 0xFF;
            b[offset + 3] =  v        & 0xFF;
        }

        void write_be(u::bytes& b, size_t offset, unsigned int v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(unsigned int));

            b[offset]     = (v >> 24) & 0xFF;
            b[offset + 1] = (v >> 16) & 0xFF;
            b[offset + 2] = (v >>  8) & 0xFF;
            b[offset + 3] =  v        & 0xFF;
        }

        void write_be(u::bytes& b, size_t offset, uint16_t v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint16_t));

            b[offset]     = (v >> 8) & 0xFF;
            b[offset + 1] =  v        & 0xFF;
        }

        void read_be(const u::bytes& b, size_t offset, uint64_t& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint64_t));

            v = (static_cast<uint64_t>(b[offset])     << 56) |
                (static_cast<uint64_t>(b[offset + 1]) << 48) |
                (static_cast<uint64_t>(b[offset + 2]) << 40) |
                (static_cast<uint64_t>(b[offset + 3]) << 32) |
                (static_cast<uint64_t>(b[offset + 4]) << 24) |
                (static_cast<uint64_t>(b[offset + 5]) << 16) |
                (static_cast<uint64_t>(b[offset + 6]) << 8)  |
                (static_cast<uint64_t>(b[offset + 7]));
        }

        void read_be(const u::bytes& b, size_t offset, int& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(int));

            v = (static_cast<int>(b[offset])     << 24) |
                (static_cast<int>(b[offset + 1]) << 16) |
                (static_cast<int>(b[offset + 2]) << 8)  |
                (static_cast<int>(b[offset + 3]));
        }

        void read_be(const u::bytes& b, size_t offset, unsigned int& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(unsigned int));

            v = (static_cast<unsigned int>(b[offset])     << 24) |
                (static_cast<unsigned int>(b[offset + 1]) << 16) |
                (static_cast<unsigned int>(b[offset + 2]) << 8)  |
                (static_cast<unsigned int>(b[offset + 3]));
        }

        void read_be(const u::bytes& b, size_t offset, uint16_t& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint16_t));

            v = (static_cast<uint16_t>(b[offset]) << 8) |
                (static_cast<uint16_t>(b[offset + 1]));
        }

        u::bytes encode_udp_wire(const udp_chunk& ch)
        {
            u::bytes r;
            r.resize(HEADER_SIZE + ch.data.size());

            //set mark
            r[0] = ch.type == udp_chunk::msg ? '!' : '@';

            //write sequence number
            write_be(r, SEQUENCE_BASE, ch.sequence);

            //write total chunks
            write_be(r, CHUNK_TOTAL_BASE, ch.total_chunks);

            //write chunk number
            write_be(r, CHUNK_BASE, ch.chunk);

            //write message
            if(!ch.data.empty()) 
                std::copy(ch.data.begin(), ch.data.end(), r.begin() + MESSAGE_BASE);

            return r;
        }

        udp_chunk decode_udp_wire(const u::bytes& b)
        {
            REQUIRE_GREATER_EQUAL(b.size(), HEADER_SIZE);

            udp_chunk ch;
            ch.valid = false;

            //read mark
            const char mark = b[0];
            if(mark != '!' && mark != '@' ) return ch;

            ch.type = mark == '!' ? udp_chunk::msg : udp_chunk::ack;

            //read sequence number
            if(b.size() < SEQUENCE_BASE + sizeof(sequence_type)) return ch;
            read_be(b, SEQUENCE_BASE, ch.sequence);

            //write total chunks 
            if(b.size() < CHUNK_TOTAL_BASE + sizeof(chunk_total_type)) return ch;
            read_be(b, CHUNK_TOTAL_BASE, ch.total_chunks);

            //read chunk number
            if(b.size() < CHUNK_BASE + sizeof(chunk_id_type)) return ch;
            read_be(b, CHUNK_BASE, ch.chunk);

            //copy message
            const size_t data_size = b.size() - HEADER_SIZE;

            if(data_size > 0)
            {
                ch.data.resize(data_size);
                std::copy(b.begin() + MESSAGE_BASE, b.end(), ch.data.begin());
            }

            ch.valid = true;
            return ch;
        }

        void udp_connection::do_send(bool force)
        {
            ENSURE(_socket);

            //check to see if a write is in progress
            if(!force && _writing) return;
            if(_out_queue.empty()) return;

            _writing = true;

            CHECK_FALSE(_out_queue.empty());

            //encode bytes to wire format
            const auto& chunk = _out_queue.front();
            _out_buffer = encode_udp_wire(chunk);

            auto p = boost::lexical_cast<short unsigned int>(chunk.port);
            udp::endpoint ep(address::from_string(chunk.host), p);

            _socket->async_send_to(ba::buffer(&_out_buffer[0], _out_buffer.size()), ep,
                    boost::bind(&udp_connection::handle_write, this,
                        ba::placeholders::error));

            ENSURE(_writing);
        }

        void udp_connection::handle_write(const boost::system::error_code& error)
        {
            //remove sent message
            //TODO: maybe do a retry?
            _out_queue.pop_front();
            _error = error;

            if(error) LOG << "error sending chunk, " << _out_queue.size() << " remaining..." << std::endl;

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

        void udp_connection::bind(const std::string& port)
        {
            LOG << "bind udp port " << port << std::endl;
            INVARIANT(_socket);
            auto p = boost::lexical_cast<short unsigned int>(port);

            _socket->open(udp::v4(), _error);
            _socket->set_option(udp::socket::reuse_address(true),_error);
            _socket->bind(udp::endpoint(udp::v4(), p), _error);

            if(_error)
                LOG << "error binding udp to port " << p << ": " << _error.message() << std::endl;

            start_read();
        }

        void udp_connection::start_read()
        {
            _socket->async_receive_from(
                   ba::buffer(_in_buffer, MAX_UDP_BUFF_SIZE), _in_endpoint,
                    boost::bind(&udp_connection::handle_read, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        }

        bool insert_chunk(const std::string& addr, const udp_chunk& c, working_udp_messages& w, u::bytes& complete_message)
        {
            auto& wms = w[addr];
            auto& wm = wms[c.sequence];
            if(wm.chunks.empty())
            {
                if(c.total_chunks == 0) return false;

                wm.chunks.resize(c.total_chunks);
                wm.set.resize(c.total_chunks);
            }

            CHECK_FALSE(wm.chunks.empty());

            auto chunk_n = c.chunk;
            auto sequence_n = c.sequence;

            if(chunk_n >= wm.chunks.size()) return false;
            if(c.total_chunks != wm.chunks.size()) return false;
            if(wm.set[chunk_n]) return false;

            wm.chunks[chunk_n] = std::move(c);
            wm.set[chunk_n] = 1;

            //if message is not complete yet, return 
            if(wm.set.count() != wm.chunks.size()) return false;

            //get total size
            size_t total_message_size = 0; 
            for(const auto& cc : wm.chunks) 
                total_message_size += cc.data.size();

            //combine chunks to recreate original message
            complete_message.resize(total_message_size);
            size_t s = 0;
            for(const auto& cc : wm.chunks) 
            {
                std::copy(cc.data.begin(), cc.data.end(), complete_message.begin() + s);
                s += cc.data.size();
            }
            CHECK_EQUAL(s, total_message_size);

            //remove message from working
            wms.erase(sequence_n);
            return true;
        }

        void udp_connection::handle_read(const boost::system::error_code& error, size_t transferred)
        {
            if(error)
            {
                u::mutex_scoped_lock l(_mutex);
                _error = error;
                LOG << "error getting message of size " << transferred  << ". " << error.message() << std::endl;
                start_read();
                return;
            }

            //get bytes
            u::bytes data;
            CHECK_LESS_EQUAL(transferred, _in_buffer.size());
            data.resize(transferred);
            std::copy(_in_buffer.begin(), _in_buffer.begin() + transferred, data.begin());

            //decode message
            udp_chunk chunk;
            chunk.valid = false;

            if(data.size() >= HEADER_SIZE) 
                chunk = decode_udp_wire(data);

            if(chunk.valid)
            {
                if(chunk.type == udp_chunk::msg)
                { 
                    //add message to in queue if we got complete message
                    endpoint ep = {
                        UDP, 
                        _in_endpoint.address().to_string(), 
                        boost::lexical_cast<std::string>(_in_endpoint.port())};

                    udp_chunk ack;
                    ack.type = udp_chunk::ack;
                    ack.host = ep.address;
                    ack.port = ep.port;
                    ack.sequence = chunk.sequence;
                    ack.total_chunks = chunk.total_chunks;
                    ack.chunk = chunk.chunk;

                    bool inserted = false;
                    {
                        u::mutex_scoped_lock l(_mutex);
                        //create ack

                        //insert chunk to message buffer
                        inserted = insert_chunk(make_address_str(ep), chunk, _in_working, data);
                        //chunk is no longer valid after insert_chunk call because a move is done.
                    }

                    //send ack
                    send(ack);
                    _io.post(boost::bind(&udp_connection::do_send, this, false));

                    if(inserted)
                    {
                        u::mutex_scoped_lock l(_in_mutex);
                        endpoint_message em = {ep, data};
                        _in_queue.emplace_push(em);
                    }
                }
                else
                {
                    //LOG << "ack: " << chunk.sequence << ":" << chunk.chunk << "/" << chunk.total_chunks << std::endl;
                    //TODO implement ACK
                }
            }

            start_read();
        }

        void udp_run_thread(udp_queue*);
        udp_queue::udp_queue(const asio_params& p) :
            _p(p), _done{false},
            _io{new ba::io_service}
        {
            REQUIRE_FALSE(_p.local_port.empty());
            bind();
            _run_thread.reset(new std::thread{udp_run_thread, this});

            INVARIANT(_io);
            INVARIANT(_con);
            INVARIANT(_run_thread);
        }

        void udp_queue::bind()
        {
            CHECK_FALSE(_con);
            INVARIANT(_io);

            _con = udp_connection_ptr{new udp_connection{_in_queue, *_io, _mutex}};
            _con->bind(_p.local_port);
            
            ENSURE(_con);
        }

        udp_queue::~udp_queue()
        {
            INVARIANT(_io);
            _io->stop();
            _done = true;
            if(_p.block) _in_queue.done();
            if(_p.wait > 0) u::sleep_thread(_p.wait);
            if(_con) _con->close();
            if(_run_thread) _run_thread->join();
        }

        bool udp_queue::send(const endpoint_message& m)
        {
            INVARIANT(_io);
            CHECK(_con);

            return _con->send(m, _p.block);
        }

        bool udp_queue::receive(endpoint_message& m)
        {
            //return true if we got message
            return _in_queue.pop(m, _p.block);
        }

        void udp_run_thread(udp_queue* q)
        {
            CHECK(q);
            CHECK(q->_io);
            while(!q->_done) 
            try
            {
                q->_io->poll();
                u::sleep_thread(THREAD_SLEEP);
            }
            catch(std::exception& e)
            {
                LOG << "error in udp thread. " << e.what() << std::endl;
            }
            catch(...)
            {
                LOG << "unknown error in udp thread." << std::endl;
            }
        }
    }
}
