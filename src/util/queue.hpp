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
#ifndef FIRESTR_UTIL_QUEUE_H
#define FIRESTR_UTIL_QUEUE_H

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "util/dbc.hpp"

namespace fire 
{
    namespace util 
    {
        struct has_size 
        {
            virtual size_t size() const = 0;
            virtual bool empty() const = 0;
        };

        template<class t>
        struct in_queue 
        {
            virtual bool pop(t& v, bool wait = false) = 0;
            virtual void done() = 0;
        };

        template<class t>
        struct out_queue 
        {
            virtual void push(const t& v) = 0; 
        };

        template<class t>
        class queue : 
            public in_queue<t>, 
            public out_queue<t>,
            public has_size
        {
            public:
                virtual void push(const t& v) 
                {
                    std::lock_guard<std::mutex> lock(_m);
                    _q.push_back(v);
                    _c.notify_one();

                    ENSURE_GREATER(_q.size(), 0);
                }

                virtual bool pop(t& v, bool wait = false)
                {
                    if(wait)
                    {
                        std::unique_lock<std::mutex> lock(_m);
                        while(_q.empty()) 
                        {
                            if(_done) return false;
                            _c.wait(lock);
                        }

                        REQUIRE_FALSE(_q.empty());

                        v = std::move(_q.front());
                        _q.pop_front();

                        return true;
                    }

                    std::lock_guard<std::mutex> lock(_m);
                    if(_q.empty()) return false;

                    v = std::move(_q.front());
                    _q.pop_front();

                    return true;
                }

                virtual void pop_front(bool wait = false)
                {
                    if(wait)
                    {
                        std::unique_lock<std::mutex> lock(_m);
                        while(_q.empty())
                        {
                            if(_done) return;
                            _c.wait(lock);
                        }
                        REQUIRE_FALSE(_q.empty());
                        _q.pop_front();
                        return;
                    }

                    std::lock_guard<std::mutex> lock(_m);
                    REQUIRE_FALSE(_q.empty());
                    _q.pop_front();
                }

                virtual t front()
                {
                    std::lock_guard<std::mutex> lock(_m);
                    REQUIRE_FALSE(_q.empty());

                    return _q.front();
                }

                virtual size_t size() const 
                { 
                    std::lock_guard<std::mutex> lock(_m);
                    return _q.size();
                }
                virtual bool empty() const 
                { 
                    std::lock_guard<std::mutex> lock(_m);
                    return _q.empty();
                }

                virtual void done()
                {
                    std::lock_guard<std::mutex> lock(_m);
                    _done = true;
                    _c.notify_all();
                }

            private:
                std::deque<t> _q;
                mutable std::mutex _m;
                mutable std::condition_variable _c;
                bool _done = false;
        };
    }
}

#endif
