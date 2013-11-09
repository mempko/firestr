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

namespace fire
{
    namespace network
    {
        namespace
        {
            const size_t BLOCK_SLEEP = 10;
            const size_t THREAD_SLEEP = 40;
            const size_t RESEND_THREAD_SLEEP = 1000;
            const size_t RESEND_TICK_THRESHOLD = 5; //resend after 10 seconds
            const size_t RESEND_THRESHOLD = 1; //resend one time
            const size_t MAX_UDP_BUFF_SIZE = 1024*500; //500k in bytes
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

        void remember_chunk(const std::string& addr, const udp_chunk& c, working_udp_messages& w)
        {
            auto& wms = w[addr];
            auto& wm = wms[c.sequence];
            if(wm.chunks.empty())
            {
                if(c.total_chunks == 0) return;

                wm.chunks.resize(c.total_chunks);
                wm.set.resize(c.total_chunks);
            }
            wm.chunks[c.chunk] = c;
            wm.set[c.chunk] = 0;
        }

        void validate_chunk(const std::string& addr, const udp_chunk& c, working_udp_messages& w)
        {
            auto chunk_n = c.chunk;
            auto sequence_n = c.sequence;

            auto& wms = w[addr];
            if(wms.count(sequence_n) == 0) return;

            auto& wm = wms[sequence_n];

            if(chunk_n >= wm.chunks.size()) return;
            if(c.total_chunks != wm.chunks.size()) return;

            wm.set[chunk_n] = 1;
            wm.ticks = 0;

            //if message is not complete yet, return 
            if(wm.set.count() != wm.chunks.size()) return;

            //remove message from working
            wms.erase(sequence_n);
        }

        void udp_connection::send(udp_chunk& c)
        {
            //push chunk to queue
            _out_queue.emplace_push(c);
        }

        size_t udp_connection::chunkify(
                const std::string& host, 
                port_type port, 
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

            u::mutex_scoped_lock l(_mutex);
            _sequence++;

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
                remember_chunk(make_address_str({ UDP, host, port}), c, _out_working);
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

            size_t chunks = chunkify(m.ep.address, m.ep.port, m.data);

            //post to do send
            _io.post(boost::bind(&udp_connection::do_send, this, false));

            //if we are blocking, block until all messages are sent
            while(block && !_out_queue.empty()) u::sleep_thread(BLOCK_SLEEP);

            return true;
        }

        void write_be_u64(u::bytes& b, size_t offset, uint64_t v)
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

        void write_be_32(u::bytes& b, size_t offset, int v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(int));

            b[offset]     = (v >> 24) & 0xFF;
            b[offset + 1] = (v >> 16) & 0xFF;
            b[offset + 2] = (v >>  8) & 0xFF;
            b[offset + 3] =  v        & 0xFF;
        }

        void write_be_u32(u::bytes& b, size_t offset, unsigned int v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(unsigned int));

            b[offset]     = (v >> 24) & 0xFF;
            b[offset + 1] = (v >> 16) & 0xFF;
            b[offset + 2] = (v >>  8) & 0xFF;
            b[offset + 3] =  v        & 0xFF;
        }

        void write_be_u16(u::bytes& b, size_t offset, uint16_t v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint16_t));

            b[offset]     = (v >> 8) & 0xFF;
            b[offset + 1] =  v        & 0xFF;
        }

        void read_be_u64(const u::bytes& b, size_t offset, uint64_t& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint64_t));

            uint64_t v8 = static_cast<unsigned char>(b[offset]);
            uint64_t v7 = static_cast<unsigned char>(b[offset + 1]);
            uint64_t v6 = static_cast<unsigned char>(b[offset + 2]);
            uint64_t v5 = static_cast<unsigned char>(b[offset + 3]);
            uint64_t v4 = static_cast<unsigned char>(b[offset + 4]);
            uint64_t v3 = static_cast<unsigned char>(b[offset + 5]);
            uint64_t v2 = static_cast<unsigned char>(b[offset + 6]);
            uint64_t v1 = static_cast<unsigned char>(b[offset + 7]);

            v = (v8 << 56) | 
                (v7 << 48) | 
                (v6 << 40) | 
                (v5 << 32) | 
                (v4 << 24) | 
                (v3 << 16) | 
                (v2 << 8)  | 
                v1;
        }

        void read_be_32(const u::bytes& b, size_t offset, int& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(int));

            int v4 = static_cast<unsigned char>(b[offset]);
            int v3 = static_cast<unsigned char>(b[offset + 1]);
            int v2 = static_cast<unsigned char>(b[offset + 2]);
            int v1 = static_cast<unsigned char>(b[offset + 3]);

            v = (v4 << 24) |
                (v3 << 16) |
                (v2 <<  8) |
                v1;
        }

        void read_be_u32(const u::bytes& b, size_t offset, unsigned int& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(unsigned int));
            unsigned int v4 = static_cast<unsigned char>(b[offset]);
            unsigned int v3 = static_cast<unsigned char>(b[offset + 1]);
            unsigned int v2 = static_cast<unsigned char>(b[offset + 2]);
            unsigned int v1 = static_cast<unsigned char>(b[offset + 3]);

            v = (v4 << 24) |
                (v3 << 16) |
                (v2 <<  8) |
                v1;
        }

        void read_be_u16(const u::bytes& b, size_t offset, uint16_t& v)
        {
            REQUIRE_GREATER_EQUAL(b.size() - offset, sizeof(uint16_t));
            uint16_t v2 = static_cast<unsigned char>(b[offset]);
            uint16_t v1 = static_cast<unsigned char>(b[offset + 1]);

            v = (v2 <<  8) | v1;
        }

        void encode_udp_wire(u::bytes& r, const udp_chunk& ch)
        {
            r.resize(HEADER_SIZE + ch.data.size());

            //set mark
            r[0] = ch.type == udp_chunk::msg ? '!' : '@';

            //write sequence number
            write_be_u64(r, SEQUENCE_BASE, ch.sequence);

            //write total chunks
            write_be_u16(r, CHUNK_TOTAL_BASE, ch.total_chunks);

            //write chunk number
            write_be_u16(r, CHUNK_BASE, ch.chunk);

            //write message
            if(!ch.data.empty()) 
                std::copy(ch.data.begin(), ch.data.end(), r.begin() + MESSAGE_BASE);
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
            read_be_u64(b, SEQUENCE_BASE, ch.sequence);

            //write total chunks 
            if(b.size() < CHUNK_TOTAL_BASE + sizeof(chunk_total_type)) return ch;
            read_be_u16(b, CHUNK_TOTAL_BASE, ch.total_chunks);

            //read chunk number
            if(b.size() < CHUNK_BASE + sizeof(chunk_id_type)) return ch;
            read_be_u16(b, CHUNK_BASE, ch.chunk);

            //copy message
            CHECK_GREATER_EQUAL(b.size(), HEADER_SIZE);
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
            encode_udp_wire(_out_buffer, chunk);

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

        void udp_connection::bind(port_type port)
        {
            LOG << "bind udp port " << port << std::endl;
            INVARIANT(_socket);

            _socket->open(udp::v4(), _error);
            _socket->set_option(udp::socket::reuse_address(true),_error);
            _socket->bind(udp::endpoint(udp::v4(), port), _error);

            if(_error)
                LOG << "error binding udp to port " << port << ": " << _error.message() << std::endl;

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
                //add message to in queue if we got complete message
                endpoint ep = { UDP, _in_endpoint.address().to_string(), _in_endpoint.port()};


                if(chunk.type == udp_chunk::msg)
                { 
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
                    u::mutex_scoped_lock l(_mutex);
                    validate_chunk(make_address_str(ep), chunk, _out_working);
                }
            }

            start_read();
        }

        using exhausted_messages = std::set<sequence_type>;

        void udp_connection::resend()
        {
            bool resent = false;
            for(auto& wms : _out_working)
            {
                exhausted_messages em;
                for(auto& wmp : wms.second)
                {
                    auto& wm = wmp.second;
                    wm.ticks++;
                    sequence_type sequence = wmp.first;
                    bool resent_m = false;
                    bool skip = wm.ticks <= RESEND_TICK_THRESHOLD;
                    bool gaps = wm.set.count() != wm.chunks.size(); 
                    if(gaps)
                    {
                        for(const auto& c : wm.chunks)
                        {
                            //skip validated
                            if(skip || wm.set[c.chunk]) continue;

                            auto mc = c;//copy
                            resent_m = true;
                            send(mc);
                        }
                    }

                    if(resent_m) 
                    {
                        resent = true;
                        wm.ticks = 0;
                        wm.resent++;
                        CHECK_FALSE(wm.chunks.empty());
                        const auto& c = wm.chunks[0];
                        LOG << "resent: " << c.host << ":" << c.port << " seq:" << sequence << " times: " << wm.resent << std::endl;
                    }

                    if(!gaps || wm.resent >= RESEND_THRESHOLD) 
                        em.insert(sequence);
                }

                //erase all exhaused messages
                for(auto sequence : em) 
                    wms.second.erase(sequence);
            }
            if(resent) _io.post(boost::bind(&udp_connection::do_send, this, false));
        }

        void udp_run_thread(udp_queue*);
        void resend_thread(udp_queue*);
        udp_queue::udp_queue(const asio_params& p) :
            _p(p), _done{false},
            _io{new ba::io_service}
        {
            REQUIRE_GREATER(_p.local_port, 0);
            _resolver.reset(new udp::resolver{*_io});
            bind();
            _run_thread.reset(new std::thread{udp_run_thread, this});
            _resend_thread.reset(new std::thread{resend_thread, this});

            INVARIANT(_io);
            INVARIANT(_con);
            INVARIANT(_resolver);
            INVARIANT(_run_thread);
            INVARIANT(_resend_thread);
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
            if(_resend_thread) _resend_thread->join();
        }

        bool udp_queue::send(const endpoint_message& m)
        {
            INVARIANT(_io);
            CHECK(_con);

            auto address = resolve(m.ep);
            if(address != m.ep.address)
            {
                endpoint_message cm = m;
                cm.ep.address = address;
                return _con->send(cm, _p.block);
            }
            return _con->send(m, _p.block);
        }

        const std::string& udp_queue::resolve(const endpoint& ep)
        {
            INVARIANT(_resolver);

            auto resolved = _rmap.find(ep.address);
            if(resolved != _rmap.end()) return resolved->second;

            udp::resolver::query q{ep.address, port_to_string(ep.port)};
            auto iter = _resolver->resolve(q);
            udp::endpoint endp = *iter;
            auto resolved_address = endp.address().to_string();
            _rmap[ep.address] = resolved_address;
            _rmap[resolved_address] = resolved_address;
            return _rmap[ep.address]; 
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

        void resend_thread(udp_queue* q)
        {
            CHECK(q);
            CHECK(q->_io);
            while(!q->_done) 
            try
            {
                {
                    u::mutex_scoped_lock l(q->_con->_mutex);
                    q->_con->resend();
                }
                u::sleep_thread(RESEND_THREAD_SLEEP);
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
