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
#include "network/util.hpp"

#ifdef _WIN64
#include <WinSock.h>  
#pragma comment (lib, "wsock32.lib")  
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#endif


namespace fire 
{
    namespace network 
    {
#ifdef _WIN64
		std::string get_lan_ip()
		{
            WSADATA d;
            if (WSAStartup(MAKEWORD(2, 2), &d)) return "";

            char host[256];
            if (gethostname(host, sizeof host)) return "";

            PHOSTENT h;
            if (!(h = gethostbyname(host))) return "";
           
            int i = 0;
            std::string ip;

            while(h->h_addr_list[i])  
            {  
                ip = inet_ntoa(*(struct in_addr *)h->h_addr_list[i]);  
                i++;
            }  
            return ip;
		}
#else
        std::string get_lan_ip()
        {
            std::string ip;
            ifaddrs *iflist, *iface;

            if (getifaddrs(&iflist) < 0) return ip;

            for (iface = iflist; iface; iface = iface->ifa_next) 
            {
                const void *addr;
                int af = iface->ifa_addr->sa_family;
                switch (af) 
                {
                    case AF_INET:
                        addr = &((sockaddr_in *)iface->ifa_addr)->sin_addr;
                        break;
                    default:
                        addr = NULL;
                }

                if (addr) 
                {
                    char addrp[INET6_ADDRSTRLEN];
                    if (inet_ntop(af, addr, addrp, sizeof(addrp)) == 0) 
                        continue;

                    ip = addrp;
                }
            }

            freeifaddrs(iflist);
            return ip;
        }
#endif
    }
}
