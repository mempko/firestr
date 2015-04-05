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

#include "network/stun_gun.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <iostream>
#include <random>
#include <functional>

#include <boost/lexical_cast.hpp>

namespace u = fire::util;

namespace fire
{
    namespace network
    {
        namespace 
        {
            const uint16_t IPV4_LENGTH = 4;
            const uint16_t IPV6_LENGTH = 16;
            const uint8_t IPV4_ATTR = 1;
            const uint8_t IPV6_ATTR = 2;

            const uint32_t HEADER_SIZE = 20;
            const uint16_t TRANSACTION_ID_LENGTH = 12;
            const uint16_t TRANSACTION_ID_LENGTH_PLUS_COOKIE = 16;
            const uint32_t COOKIE = 0x2112A442;

            const uint16_t MAPPED_ADDRESS_ATTR   = 0x0001;
            const uint16_t RESPONSE_ADDRESS_ATTR = 0x0002;
            const uint16_t CHANGE_REQUEST_ATR   = 0x0003;
            const uint16_t SOURCE_ADDRESS_ATR   = 0x0004;
            const uint16_t CHANGED_ADDRESS_ATR  = 0x0005; 
            const uint16_t USERNAME_ATTR = 0x0006;
            const uint16_t MESSAGE_INTEGRITY_ATTR = 0x0008;
            const uint16_t ERRORCODE_ATTR = 0x0009;
            const uint16_t UNKNOWN_ATTRIBUTES_ATTR = 0x000A;
            const uint16_t REALM_ATTR = 0x0014;
            const uint16_t NONCE_ATTR = 0x0015;
            const uint16_t XOR_MAPPED_ADDRESS_ATTR = 0x0020;
            const uint16_t PADDING_ATTR = 0x0026;
            const uint16_t RESPONSE_PORT_ATTR = 0x0027;

        }

        enum stun_class
        {
            request=0x00,
            indication=0x01,
            success=0x02,
            failure=0x03,
            invalid_class = 0xff
        };

        enum stun_message
        {
            binding = 0x0001,
            invalid_msg = 0xffff
        };

        void out_header(QDataStream& out)
        {
            stun_message type = binding;
            stun_class cls = request;
            uint16_t type_field=0;

            // merge message type and class, and the leading zero bits into a 16-bit field
            type_field =  (type & 0x0f80) << 2;
            type_field |= (type & 0x0070) << 1;
            type_field |= (type & 0x000f);
            type_field |= (cls & 0x02) << 7;
            type_field |= (cls & 0x01) << 4;

            out << type_field; 

            //placeholder for length, we will go back and add it in
            uint16_t length_placeholder=0;
            out << length_placeholder;
        }

        struct transaction_id
        {
            uint8_t v[TRANSACTION_ID_LENGTH];
        };

        transaction_id out_tranaction_id(QDataStream& out)
        {
            std::uniform_int_distribution<int> distribution(0, 255);
            std::mt19937 engine; 
            auto generator = std::bind(distribution, engine);

            out << COOKIE;

            transaction_id id;
            for(int i = 0; i < TRANSACTION_ID_LENGTH; ++i)
            {
                id.v[i] = generator();
                out << id.v[i];
            }
            return id;
        }

        void out_attribute(QDataStream& out, uint16_t type, char* data, uint16_t size)
        {
            //attributes start on a 4 byte boundary
            char padding[4] = {0};
            size_t remainder = size % 4;  
            size_t padding_size = remainder ? 4 - remainder : 0;

            out << type << size;
            if(size > 0) out.writeRawData(data, size);
            if(padding_size > 0) out.writeRawData(padding, padding_size);
        }

        void out_empty_change_request(QDataStream& out)
        {
            uint32_t data = 0; //as done in stund code, we add an empty change request
                               //for compatibility with JSTUN servers
            out_attribute(out, CHANGE_REQUEST_ATR, reinterpret_cast<char*>(&data), sizeof(data));
        }

        void fix_length_field(QDataStream& out)
        {
            auto device = out.device();
            CHECK(device);
            auto size = device->pos();
            auto pos = size;

            if(size < HEADER_SIZE) size = 0;
            else size -= HEADER_SIZE;

            device->seek(2);
            uint16_t s = size;
            out << s;
            device->seek(pos);
        }

        void stun_gun::send_stun_request()
        try
        {
            //attemp to connect to stun server
            QByteArray datagram;
            QDataStream out(&datagram, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_8);
            out.setByteOrder(QDataStream::BigEndian);

            //create request
            out_header(out);
            out_tranaction_id(out);
            out_empty_change_request(out);
            fix_length_field(out);

            //send request
            QHostAddress stun_host{_stun_server.c_str()};
            _socket->writeDatagram(datagram, stun_host, boost::lexical_cast<int>(_stun_port));
        }
        catch(std::exception& e)
        {
            LOG << "stun error: " << e.what() << std::endl;
            _state = stun_failed;
        }
        catch(...)
        {
            LOG << "stun error: stun failed for unknown reason" << std::endl;
            _state = stun_failed;
        }

        stun_gun::stun_gun(QObject* parent, const std::string& stun_server, const std::string stun_port, const std::string port) :
            QObject{parent},
            _state{stun_in_progress},
            _stun_server{stun_server}, 
            _stun_port{stun_port}, 
            _int_port{port}
        {
            REQUIRE(parent);

            _socket = new QUdpSocket{this};

            connect(_socket, SIGNAL(readyRead()), this, SLOT(got_response()));
            connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(error(QAbstractSocket::SocketError)));

            int src_port = boost::lexical_cast<int>(_int_port);
            _socket->bind(src_port);

            INVARIANT(_socket);
        }

        stun_state stun_gun::state() const
        {
            return _state;
        }

        const std::string& stun_gun::stun_server() const
        {
            return _stun_server;
        }

        const std::string& stun_gun::stun_port() const
        {
            return _stun_port;
        }

        const std::string& stun_gun::internal_port() const
        {
            return _int_port;
        }
        
        const std::string& stun_gun::external_ip() const
        {
            return _ext_ip;
        }

        const std::string& stun_gun::external_port() const
        {
            return _ext_port;
        }

        struct raw_stun_header
        {
            uint16_t type;
            uint16_t length;
            uint32_t cookie;
            transaction_id id;
        };

        stun_class read_header(QDataStream& in, raw_stun_header& h)
        {
            in >> h.type >> h.length >> h.cookie;
            in.readRawData(reinterpret_cast<char*>(&h.id), sizeof(h.id));

            if(h.cookie != COOKIE) return failure;

            uint16_t masked_class = h.type & 0x0110;

            stun_class c;
            switch(masked_class)
            {
                case 0x0000: c = request; break;
                case 0x0010: c = indication; break;
                case 0x0100: c = success; break;
                case 0x0110: c = failure; break;
                default: c = failure;
            }
            return c;
        }

        struct raw_attribute
        {
            uint16_t type;
            uint16_t size;
            uint16_t offset;
            u::bytes data;
        };
        using attributes = std::map<uint16_t, raw_attribute>;

        attributes read_attribytes(QDataStream& in)
        {
            attributes as;

            size_t size = in.device()->size();
            size_t read = HEADER_SIZE;
            bool ok = true;
            while(ok && read < size)
            {
                raw_attribute a;
                int padding_length = 0;

                in >> a.type >> a.size;
                if(in.status() != QDataStream::Ok) { ok = false; continue;}

                a.offset = in.device()->pos();

                size_t remainder = a.size % 4;
                if(remainder > 0) padding_length = 4 - remainder;

                a.data.resize(a.size);
                in.readRawData(&a.data[0], a.size);
                if(in.status() != QDataStream::Ok) { ok = false; continue;}

                in.skipRawData(padding_length);
                if(in.status() != QDataStream::Ok) { ok = false; continue;}

                as[a.type] = a;
                read += sizeof(a.type) + sizeof(a.size) + a.size + padding_length;
            }

            return as;
        }

        struct mapped_address
        {
            std::string ip;
            std::string port;
        };

        bool read_mapped_address(attributes& attrs, mapped_address& mapped, const raw_stun_header& header)
        {
            bool xored = attrs.count(XOR_MAPPED_ADDRESS_ATTR) > 0;
            if(!xored && attrs.count(MAPPED_ADDRESS_ATTR) == 0) return false; 

            auto& a = attrs[xored ? XOR_MAPPED_ADDRESS_ATTR : MAPPED_ADDRESS_ATTR];
            auto& data = a.data;

            QByteArray ba{&data[0], static_cast<int>(data.size())};
            QDataStream in{&ba, QIODevice::ReadOnly};

            uint8_t id;
            uint16_t port;
            uint8_t ip6[IPV6_LENGTH];
            uint32_t ip4;

            //skip over zero byte
            char zero_b;
            in.readRawData(&zero_b, 1);

            in >> id >> port; 

            const bool is_ipv4 = id == IPV4_ATTR;
            if(is_ipv4) in >> ip4;
            else in.readRawData(reinterpret_cast<char*>(&ip6[0]), IPV6_LENGTH);

            if(xored)
            {
                size_t ip_size = is_ipv4 ? IPV4_LENGTH : IPV6_LENGTH;

                uint8_t* port_p = reinterpret_cast<uint8_t*>(&port);
                uint8_t* ip_p = is_ipv4 ? 
                    reinterpret_cast<uint8_t*>(&ip4) :
                    reinterpret_cast<uint8_t*>(&ip6[0]);  

                port_p[0] ^= header.id.v[0];
                port_p[1] ^= header.id.v[1];

                for(size_t i = 0; i < ip_size; ++i)
                    ip_p[i] ^= header.id.v[i];
            }

            mapped.port = boost::lexical_cast<std::string>(port);

            if(is_ipv4)
            {
                uint8_t* ip_p = reinterpret_cast<uint8_t*>(&ip4);

                std::stringstream ips;
                ips << static_cast<int>(ip_p[3]) << ".";
                ips << static_cast<int>(ip_p[2]) << ".";
                ips << static_cast<int>(ip_p[1]) << ".";
                ips << static_cast<int>(ip_p[0]);
                mapped.ip = ips.str();
            }
            else
            {
                uint8_t* ip_p = reinterpret_cast<uint8_t*>(&ip6[IPV6_LENGTH-1]);
                std::stringstream ips;
                for(int i = IPV6_LENGTH-2; i >= 0; i--) ips << ip_p[i];
                mapped.ip = ips.str();
            }

            return true;
        }

        void stun_gun::got_response()
        try
        { 
            INVARIANT(_socket);

            LOG << "got stun response!" << std::endl;
            QByteArray datagram;

            do 
            {
                datagram.resize(_socket->pendingDatagramSize());
                _socket->readDatagram(datagram.data(), datagram.size());
            } 
            while (_socket->hasPendingDatagrams());

            QDataStream in{&datagram, QIODevice::ReadOnly};
            in.setVersion(QDataStream::Qt_4_3);
            in.setByteOrder(QDataStream::BigEndian);

            //read header
            raw_stun_header header;
            auto cls = read_header(in, header);
            if(cls != success) 
            {
                _state = stun_failed;
                return;
            }

            //read mapped port
            auto attrs = read_attribytes(in);
            mapped_address address;
            if(!read_mapped_address(attrs, address, header)) 
            {
                _state = stun_failed;
                return;
            }

            _ext_ip = address.ip;
            _ext_port = address.port;
            _state = stun_success;
            _socket->close();
        }
        catch(std::exception& e)
        {
            LOG << "stun error: " << e.what() << std::endl;
            _state = stun_failed;
        }
        catch(...)
        {
            LOG << "stun error: stun failed for unknown reason" << std::endl;
            _state = stun_failed;
        }

        void stun_gun::error(QAbstractSocket::SocketError e)
        {
            switch (e) 
            {
                case QAbstractSocket::RemoteHostClosedError:
                    LOG << "error: host closed" << std::endl;
                    break;
                case QAbstractSocket::HostNotFoundError:
                    LOG << "error: host not found" << std::endl;
                    break;
                case QAbstractSocket::ConnectionRefusedError:
                    LOG << "error: connection refused" << std::endl;
                    break;
                default:
                    LOG << "error: " << _socket->errorString().toUtf8().constData() << std::endl;
            }
            _state = stun_failed;
        }
    }
}

