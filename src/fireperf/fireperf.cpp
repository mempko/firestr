/*
 * Copyright (C) 2015  Maxim Noah Khailo
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

#include <string>
#include <cstdlib>
#include <fstream>
#include <termios.h>
#include <chrono>

#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "network/connection_manager.hpp"
#include "message/message.hpp"
#include "messages/greeter.hpp"
#include "util/bytes.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <botan/botan.h>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;
namespace bf = boost::filesystem;
namespace n = fire::network;
namespace m = fire::message;
namespace ms = fire::messages;
namespace u = fire::util;

namespace
{
    const size_t THREAD_SLEEP = 100; //in milliseconds
    const size_t POOL_SIZE = 1; //small pool size for now
    n::port_type SRC_PORT = 7170;
    n::port_type DST_PORT = 7171;
    const std::string DST_ADDR = "udp://localhost:7171";
}

po::options_description create_descriptions()
{
    po::options_description d{"Options"};

    const std::string host = ip::host_name();

    d.add_options()
        ("help", "prints help")
        ("messages", po::value<int>()->default_value(100000), "Number of messages")
        ("robust", po::value<bool>()->default_value(true), "Are messages robust?")
        ("size", po::value<int>()->default_value(512), "Message size in bytes");

    return d;
}

po::variables_map parse_options(int argc, char* argv[], po::options_description& desc)
{
    po::variables_map v;
    po::store(po::parse_command_line(argc, argv, desc), v);
    po::notify(v);

    return v;
}

int main(int argc, char *argv[])
{
    auto desc = create_descriptions();
    auto vm = parse_options(argc, argv, desc);
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    Botan::LibraryInitializer init;

    auto iterations = vm["messages"].as<int>();
    auto total_iterations = iterations;
    auto robust = vm["robust"].as<bool>();
    size_t bytes_per_message = vm["size"].as<int>();

    n::connection_manager src{POOL_SIZE, static_cast<n::port_type>(SRC_PORT)};
    n::connection_manager dst{POOL_SIZE, static_cast<n::port_type>(DST_PORT)};


    auto data = u::to_bytes(std::string(bytes_per_message, 'm'));

    u::bytes got_data;

    n::endpoint ep;

    auto start = std::chrono::high_resolution_clock::now();
    while(iterations)
    try
    {
        src.send(DST_ADDR, data, robust);
        while(!dst.receive(ep, got_data)); //spin until we get something

        CHECK(got_data == data);
        iterations--;
    }
    catch(std::exception& e)
    {
        LOG << "error getting message: " << e.what() << std::endl;
    }
    catch(...)
    {
        LOG << "unknown error getting message: " << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    auto sec = duration / 1000000000.0;
    auto total_bytes_sent = total_iterations * data.size();
    auto time_per_byte = duration / total_bytes_sent;
    auto kb_per_sec = (total_bytes_sent/1024) / sec;
    auto time_per_message = (duration / total_iterations) / 1000.0;
    std::cout << "messages: " << total_iterations << " time: " << sec << "s" << std::endl;
    std::cout << "bytes per message: " << bytes_per_message << std::endl;
    std::cout << "sent bytes: " << total_bytes_sent<< std::endl;
    std::cout << "kb per sec: " << kb_per_sec << std::endl;
    std::cout << "time/byte: " << time_per_byte << "ns" << std::endl;
    std::cout << "time/message: " << time_per_message << "ms" << std::endl;

}
