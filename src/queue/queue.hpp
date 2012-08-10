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
#ifndef FIRESTR_QUEUE_H
#define FIRESTR_QUEUE_H

#include <deque>
#include <thread>
#include <mutex>

#include "util/dbc.hpp"

namespace fire 
{
    namespace queue 
    {
        template<class t>
        class queue
        {
            public:
                void push(const t& v) 
                {
                    std::lock_guard<std::mutex> lock(_m);
                    _q.push_back(v);

                    ENSURE_GREATER(_q.size(), 0);
                }

                bool pop(t& v)
                {
                    std::lock_guard<std::mutex> lock(_m);
                    if(_q.empty()) return false;

                    v = std::move(_q.front());
                    _q.pop_front();

                    return true;
                }

                t pop()
                {
                    std::lock_guard<std::mutex> lock(_m);
                    REQUIRE_FALSE(empty());

                    t v = std::move(_q.front());
                    _q.pop_front();

                    return v;
                }

                size_t size() const { return _q.size();}
                bool empty() const { return _q.empty();}

            private:
                std::deque<t> _q;
                std::mutex _m;
        };
    }
}

#endif
