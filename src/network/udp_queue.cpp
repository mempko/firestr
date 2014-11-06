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

#include "network/udp_queue.hpp"
#include "util/thread.hpp"
#include "util/mencode.hpp"
#include "util/string.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <sstream>
#include <functional>
#include <boost/bind.hpp>

namespace u = fire::util;
namespace ba = boost::asio;
using namespace boost::asio::ip;

namespace fire
{
    namespace network
    {
        namespace
        {
            const size_t MAX_FLIGHT = 10;
            const size_t ERASE_COUNT = 5;
            const size_t BLOCK_SLEEP = 10;
            const size_t THREAD_SLEEP = 40;
            const size_t RESEND_THREAD_SLEEP = 1000;
            const size_t RESEND_TICK_THRESHOLD = 1; //resend after 1 seconds
            const size_t RESEND_THRESHOLD = 5; //purge message after 5 seconds
            const size_t UDP_PACKET_SIZE = 512; //in bytes
            const size_t MAX_UDP_BUFF_SIZE = UDP_PACKET_SIZE*2; 
            const size_t SEQUENCE_BASE = 1;
            const size_t CHUNK_TOTAL_BASE = SEQUENCE_BASE + sizeof(sequence_type);
            const size_t CHUNK_BASE = CHUNK_TOTAL_BASE + sizeof(chunk_total_type);
            const size_t MESSAGE_BASE = CHUNK_BASE + sizeof(chunk_id_type);

            //<mark> <sequence num> <chunk total> <chunk>
            const size_t HEADER_SIZE = MESSAGE_BASE;
            const size_t UDP_CHuNK_SIZE = UDP_PACKET_SIZE - HEADER_SIZE; //in bytes

            const size_t MAX_CHUNKS = std::pow(2,sizeof(chunk_total_type)*8);
            const size_t MAX_MESSAGE_SIZE = MAX_CHUNKS * UDP_CHuNK_SIZE;
        }

        udp_queue_ptr create_udp_queue(const asio_params& p)
        {
            return udp_queue_ptr{new udp_queue{p}};
        }

        udp_connection::udp_connection(
                endpoint_queue& in,
                boost::asio::io_service& io) :
            _in_buffer(MAX_UDP_BUFF_SIZE),
            _in_queue(in),
            _io(io),
            _socket{new udp::socket{io}},
            _writing{false}
        {
            boost::system::error_code error;
            _socket->open(udp::v4(), error);
            _next_out_queue = _out_queues.end();

            INVARIANT(_socket);
        }

        void udp_connection::close()
        {
            _io.post(boost::bind(&udp_connection::do_close, this));
        }

        void udp_connection::do_close()
        {
            INVARIANT(_socket);
            _socket->close();
            _writing = false;
        }

        working_udp_chunks* find_wm(const std::string& addr, sequence_type sequence, working_udp_messages& w)
        {
            auto i = w.find(addr);
            if(i == w.end()) return nullptr;

            auto mi = i->second.find(sequence);
            if(mi == i->second.end()) return nullptr;

            return &mi->second;
        }

        working_udp_chunks* init_working(const std::string& addr, working_udp_messages& w, const udp_chunk& c)
        {
            auto& wms = w[addr];
            auto& wm = wms[c.sequence];
            if(!wm.chunks.empty()) return &wm;
            if(c.total_chunks == 0) return nullptr;

            wm.chunks.resize(c.total_chunks);
            wm.set.resize(c.total_chunks);
            wm.sent.resize(c.total_chunks);
            return &wm;
        }

        void remember_chunk(working_udp_chunks& wm ,const udp_chunk& c)
        {
            REQUIRE_RANGE(c.chunk, 0, c.total_chunks);
            wm.chunks[c.chunk] = c;
            wm.sent[c.chunk] = 0;
        }

        void sent_chunk(working_udp_chunks& wm , const udp_chunk& c)
        {
            REQUIRE_RANGE(c.chunk, 0, c.total_chunks);

            wm.chunks[c.chunk] = c;
            wm.sent[c.chunk] = 1;
            if(wm.queued > 0) wm.queued--;

            bool robust = c.type == udp_chunk::msg;
            if(robust) wm.in_flight++;
        }

        void sent_chunk(const std::string& addr, const udp_chunk& c, working_udp_messages& w)
        {
            REQUIRE(c.type != udp_chunk::ack);
            REQUIRE_FALSE(c.resent);

            auto wmp = init_working(addr, w, c);
            if(!wmp) return;
            sent_chunk(*wmp, c);
        }

        void validate_chunk(const std::string& addr, const udp_chunk& c, working_udp_messages& w)
        {
            REQUIRE(c.type == udp_chunk::ack);

            auto chunk_n = c.chunk;
            auto sequence_n = c.sequence;

            auto& wms = w[addr];
            auto wmi = wms.find(sequence_n);
            if(wmi == wms.end()) return;

            auto& wm = wmi->second;

            if(chunk_n >= wm.chunks.size()) return;
            if(c.total_chunks != wm.chunks.size()) return;
            if(wm.set[chunk_n]) return;

            wm.set[chunk_n] = 1;
            wm.ticks = 0;
            if(wm.in_flight > 0 ) wm.in_flight--;

            //if message is not complete yet, return 
            if(wm.set.count() != wm.chunks.size()) return;

            CHECK_EQUAL(wm.sent.count(), wm.chunks.size());
            CHECK_EQUAL(wm.in_flight, 0);
            CHECK_EQUAL(wm.queued, 0);

            //remove message from working
            wms.erase(sequence_n);
        }

        void udp_connection::send_right_away(udp_chunk& c)
        {
            //push chunk to queue
            _out_queue.emplace_push(c);
        }

        using exhausted_messages = std::set<sequence_type>;

        void udp_connection::queue_chunks(working_udp_messages& w)
        {
            for(auto& wms : w)
            {
                exhausted_messages em;
                for(auto& wm : wms.second)
                {
                    CHECK_FALSE(wm.second.chunks.empty());
                    bool remove_non_robust = queue_chunks(wm.second);
                    if(remove_non_robust) em.insert(em.end(), wm.second.chunks[0].sequence);
                }

                //cleanup exhausted messages that are not robust
                for(auto sequence : em) wms.second.erase(sequence);
            }
        }

        bool all_sent(working_udp_chunks& wm)
        {
            return wm.sent.count() == wm.chunks.size();
        }

        bool udp_connection::queue_chunks(working_udp_chunks& wm)
        {
            REQUIRE_FALSE(wm.chunks.empty());
            bool robust = wm.chunks[0].type == udp_chunk::msg;

            auto in_flight = robust ? wm.in_flight : 0;

            if(in_flight + wm.queued > MAX_FLIGHT) return false;
            auto t = MAX_FLIGHT - (in_flight + wm.queued);
            while(t > 0 && wm.next_send < wm.chunks.size())
            {
                queue_chunk(wm.chunks[wm.next_send]);

                wm.next_send++;
                wm.queued++;
                t--;
            }

            return !robust && all_sent(wm);
        }

        void udp_connection::queue_chunk(udp_chunk& c)
        {
            auto p = _out_queue_map.insert(std::make_pair(c.sequence, _out_queues.end()));
            if(p.second)
            {
                auto addr = make_address_str({ UDP, c.host, c.port});
				queue_ring_item ri = { addr, chunk_queue(), c.sequence, ERASE_COUNT };
                auto i = _out_queues.insert(_out_queues.end(), ri);
                p.first->second = i;
            }
            p.first->second->queue.emplace_push(c);
        }

        bool udp_connection::next_chunk_incr()
        {
            if(_out_queues.empty())
            {
                _next_out_queue = _out_queues.end();
                return false;
            }

            //if at end, go to beginning
            if(_next_out_queue == _out_queues.end()) 
                _next_out_queue = _out_queues.begin();
            else _next_out_queue++;

            //if was at one before end and hit end, go to the beginning
            if(_next_out_queue == _out_queues.end()) 
                _next_out_queue = _out_queues.begin();
            return true;
        }

        void udp_connection::queue_next_chunk()
        {
            //queue chunks from working messages
            queue_chunks(_out_working);

            //do round robin
            if(!next_chunk_incr()) return;
            auto end = _next_out_queue;

            do
            {
                udp_chunk c;
                if(_next_out_queue->queue.pop(c))
                {
                    _out_queue.emplace_push(c);
                    break;
                } 
                next_chunk_incr();
            }
            while(_next_out_queue != end);

            //collect garbage
            for(auto j = _out_queues.begin();j != _out_queues.end(); j++)
            {
                if(!j->queue.empty()) continue;

                j->erase_count--;
                if(j->erase_count > 0) continue;

                _out_queue_map.erase(j->sequence);
                bool is_next = _next_out_queue == j;
                j = _out_queues.erase(j);

                //move next if we erased it
                if(is_next) _next_out_queue = j;
            }
        }

        size_t udp_connection::chunkify(endpoint_message m)
        {
            REQUIRE_FALSE(m.data.empty());

            const auto& host = m.ep.address;
            auto port = m.ep.port;
            const auto& b = m.data;

            int total_chunks = b.size() < UDP_CHuNK_SIZE ? 
                1 : (b.size() / UDP_CHuNK_SIZE) + 1;

            CHECK_GREATER(total_chunks, 0);

            int chunk = 0;
            size_t s = 0;
            size_t e = std::min(b.size(), UDP_CHuNK_SIZE);

            CHECK_GREATER(total_chunks, 0);

            //update sequence
            _sequence++;

            working_udp_chunks* wmp = nullptr;
            auto addr = make_address_str({ UDP, host, port});
            while(s < b.size())
            {
                size_t size = e - s;
                CHECK_GREATER(size, 0);

                //create chunk
                udp_chunk c;
                c.type = m.robust ? udp_chunk::msg : udp_chunk::qmsg;
                c.host = host;
                c.port = port;
                c.sequence = _sequence;
                c.total_chunks = total_chunks;
                c.chunk = chunk;
                c.data.resize(size);
                std::copy(b.begin() + s, b.begin() + e, c.data.begin());

                //add to working
                if(!wmp) wmp = init_working(addr, _out_working, c);
                if(!wmp) break;
                remember_chunk(*wmp, c);

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

            _io.post(boost::bind(&udp_connection::chunkify, this, m));

            //post to do send
            _io.post(boost::bind(&udp_connection::do_send, this));

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
            switch(ch.type)
            {
                case udp_chunk::msg: r[0] = '!'; break;
                case udp_chunk::qmsg: r[0] = '='; break;
                case udp_chunk::ack: r[0] = '@'; break;
                default: CHECK(false && "missed case");
            }

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
            switch(mark)
            {
                case '!': ch.type = udp_chunk::msg; break;
                case '=': ch.type = udp_chunk::qmsg; break;
                case '@': ch.type = udp_chunk::ack; break;
                default: return ch;
            }

            //read sequence number
            if(b.size() < SEQUENCE_BASE + sizeof(sequence_type)) return ch;
            read_be_u64(b, SEQUENCE_BASE, ch.sequence);

            //write total chunks 
            if(b.size() < CHUNK_TOTAL_BASE + sizeof(chunk_total_type)) return ch;
            read_be_u16(b, CHUNK_TOTAL_BASE, ch.total_chunks);

            //cannot be more than max chunks, this should be impossible because
            //total_chunks should be a unsigned short
            CHECK_LESS_EQUAL(ch.total_chunks, MAX_CHUNKS);

            //read chunk number
            if(b.size() < CHUNK_BASE + sizeof(chunk_id_type)) return ch;
            read_be_u16(b, CHUNK_BASE, ch.chunk);

            //copy message
            CHECK_GREATER_EQUAL(b.size(), HEADER_SIZE);
            const size_t data_size = b.size() - HEADER_SIZE;

            if(data_size > 0)
            {
                if(data_size > MAX_UDP_BUFF_SIZE) return ch;
                ch.data.resize(data_size);
                std::copy(b.begin() + MESSAGE_BASE, b.end(), ch.data.begin());
            }

            ch.valid = true;
            return ch;
        }

        void udp_connection::do_send()
        {
            ENSURE(_socket);

            //get next chunk
            if(_out_queue.empty()) queue_next_chunk();
            if(_out_queue.empty()) return;
            CHECK_FALSE(_out_queue.empty());

            //encode bytes to wire format
            udp_chunk chunk;
            bool got = _out_queue.pop(chunk);
            CHECK(got);

            u::bytes_ptr out_buffer{new u::bytes};
            encode_udp_wire(*out_buffer, chunk);
            _stats.bytes_sent += out_buffer->size();

            //async send chunk
            udp::endpoint ep(address::from_string(chunk.host), chunk.port);
            _socket->async_send_to(ba::buffer(out_buffer->data(), out_buffer->size()), ep,
                    boost::bind(&udp_connection::handle_write, this, out_buffer,
                        ba::placeholders::error));

            //ignore acks or resends
            if(!chunk.resent && chunk.type != udp_chunk::ack)
            {
                auto addr = make_address_str({ UDP, chunk.host, chunk.port});
                sent_chunk(addr, chunk, _out_working);
            }

            //queue next message
            if(_out_queue.empty()) queue_next_chunk();
            if(_out_queue.empty()) return;

            //post send
            _io.post(boost::bind(&udp_connection::do_send, this));
        }

        void udp_connection::handle_write(u::bytes_ptr buff, const boost::system::error_code& error)
        {
            _error = error;
            if(error) LOG << "error sending chunk, " << _out_queue.size() << " remaining..." << std::endl;
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
            REQUIRE(c.type == udp_chunk::msg || c.type == udp_chunk::qmsg);

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
                _error = error;
                LOG << "error getting message of size " << transferred  << ". " << error.message() << std::endl;
                start_read();
                return;
            }

            //get bytes
            CHECK_LESS_EQUAL(transferred, _in_buffer.size());
            u::bytes data(transferred);
            std::copy(_in_buffer.begin(), _in_buffer.begin() + transferred, data.begin());

            _stats.bytes_recv += transferred;

            //decode message
            udp_chunk chunk;
            chunk.valid = false;

            if(data.size() >= HEADER_SIZE) 
                chunk = decode_udp_wire(data);

            if(chunk.valid)
            {
                //add message to in queue if we got complete message
                endpoint ep = { UDP, _in_endpoint.address().to_string(), _in_endpoint.port()};

                if(chunk.type != udp_chunk::ack)
                { 
                    bool robust = chunk.type == udp_chunk::msg;
                    if(robust)
                    {
                        udp_chunk ack;
                        ack.type = udp_chunk::ack;
                        ack.host = ep.address;
                        ack.port = ep.port;
                        ack.sequence = chunk.sequence;
                        ack.total_chunks = chunk.total_chunks;
                        ack.chunk = chunk.chunk;
                        CHECK(ack.data.empty());

                        //send ack
                        send_right_away(ack);
                    }

                    //insert chunk to message buffer
                    bool inserted = insert_chunk(make_address_str(ep), chunk, _in_working, data);
                    //chunk is no longer valid after insert_chunk call because a move is done.

                    if(inserted)
                    {
                        endpoint_message em{ep, data, robust};
                        _in_queue.emplace_push(em);
                    }

                }
                else validate_chunk(make_address_str(ep), chunk, _out_working);

                _io.post(boost::bind(&udp_connection::do_send, this));
            }
            start_read();
        }

        size_t udp_connection::resend(working_udp_chunks& wm)
        {
            if(wm.set.count() == wm.chunks.size()) return 0;

            size_t resent_m = 0;
            size_t cnt = 0;
            for(const auto& c : wm.chunks)
            {
                if(cnt >= wm.next_send) break;
                cnt++;
                //skip validated or not sent
                if(wm.set[c.chunk] || !wm.sent[c.chunk]) continue;
                resent_m++;
                if(resent_m > MAX_FLIGHT) break;

                udp_chunk mc = c;//copy
                mc.resent = true;
                _stats.dropped++;
                queue_chunk(mc);
            }
            return resent_m;
        }

        void udp_connection::resend()
        {
            bool resent = false;
            for(auto& wms : _out_working)
            {
                exhausted_messages em;
                for(auto& wmp : wms.second)
                {
                    auto& wm = wmp.second;
                    CHECK(!wm.chunks.empty());

                    wm.ticks++;

                    sequence_type sequence = wmp.first;

                    //check if we should try to resend
                    bool skip = wm.ticks <= RESEND_TICK_THRESHOLD;
                    bool robust = wm.chunks[0].type == udp_chunk::msg;

                    //walk working message and resend all chunks that never got
                    //acked
                    if(robust && !skip && resend(wm) > 0) 
                    {
                        resent = true;
                        CHECK_FALSE(wm.chunks.empty());
                    } 

                    bool erase = robust ?  wm.ticks >= RESEND_THRESHOLD : all_sent(wm);
                    if(erase) em.insert(em.end(), sequence);
                }

                //erase all exhaused messages
                for(auto sequence : em) wms.second.erase(sequence);
            }

            if(resent) _io.post(boost::bind(&udp_connection::do_send, this));
        }

        const udp_stats& udp_connection::stats() const 
        {
            return _stats;
        }

        void udp_run_thread(udp_queue*);
        void resend_thread(udp_queue*);
        udp_queue::udp_queue(const asio_params& p) :
            _p(p), 
            _io{new ba::io_service},
            _done{false}
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

            _con = udp_connection_ptr{new udp_connection{_in_queue, *_io}};
            _con->bind(_p.local_port);
            
            ENSURE(_con);
        }

        udp_queue::~udp_queue()
        {
            INVARIANT(_io);
            _done = true;
            _io->stop();
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

        const udp_stats& udp_queue::stats() const 
        {
            CHECK(_con);
            return _con->stats();
        }

        void udp_run_thread(udp_queue* q)
        {
            CHECK(q);
            CHECK(q->_io);
            while(!q->_done) 
            try
            {
                q->_io->run();
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
                q->_io->post(boost::bind(&udp_connection::resend, q->_con));
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
