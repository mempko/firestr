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
#ifndef FIRESTR_NETWORK_STUNGUN_H
#define FIRESTR_NETWORK_STUNGUN_H

#ifndef Q_MOC_RUN
#include "util/bytes.hpp"
#endif

#include <QObject>
#include <QtGui>
#include <QtNetwork>
#include <string>
#include <memory>
#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif

namespace fire
{
    namespace network
    {
        enum stun_state { stun_in_progress, stun_failed, stun_success};
        class stun_gun : public QObject
        {
            Q_OBJECT
            public:
                stun_gun(QObject* parent, const std::string& stun_server, const std::string stun_port, const std::string port);

            public:
                void send_stun_request();

            public:
                stun_state state() const;

                const std::string& stun_server() const;
                const std::string& stun_port() const;
                const std::string& internal_port() const;
                const std::string& external_ip() const;
                const std::string& external_port() const;

            public slots:
                void got_response();
                void error(QAbstractSocket::SocketError);

            private:

                QUdpSocket* _socket;
                stun_state _state;
                std::string _stun_server;
                std::string _stun_port;
                std::string _int_port;
                std::string _ext_ip;
                std::string _ext_port;
        };

        using stun_gun_ptr = std::shared_ptr<stun_gun>;
    }
}
#endif

