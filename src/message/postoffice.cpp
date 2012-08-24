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

#include "message/postoffice.hpp"
#include "util/dbc.hpp"

namespace fire
{
    namespace message
    {
        void send_thread(post_office* o)
        {
            REQUIRE(o);
            while(!o->_done)
            {
                for(auto p : o->_boxes)
                {
                    auto wp = p.second;
                    auto sp = wp.lock();
                    if(!sp) continue;

                    message m;
                    if(!sp->pop_outbox(m)) continue;

                    m.meta.from.push_front(sp->address());

                    CHECK_EQUAL(m.meta.from.size(), 1);
                    std::cerr << o->_address << " sending from: " << sp->address() << " to: " << m.meta.to << std::endl;
                    o->send(m);
                }
            }
        }
         
        post_office::post_office() :
                _address{},
                _boxes{},
                _offices{},
                _parent{},
                _done{false}
        {
            _send_thread.reset(new std::thread{send_thread, this});

            INVARIANT(_send_thread);
        }

        post_office::post_office(const std::string& a) : 
                _address{a},
                _boxes{},
                _offices{},
                _parent{},
                _done{false}
        {
            _send_thread.reset(new std::thread{send_thread, this});

            INVARIANT(_send_thread);
        }

        post_office::~post_office()
        {
            INVARIANT(_send_thread);
            _done = true;
            _send_thread->join();
        }

        const std::string& post_office::address() const
        {
            return _address;
        }

        void post_office::address(const std::string& a)
        {
            _address = a;
        }

        bool post_office::send(message m)
        {
            metadata& meta = m.meta;
            if(meta.to.empty()) return false;

            if(meta.to.size() > 1 && meta.to.front() == _address)
                meta.to.pop_front();

            //route to child post
            if(meta.to.size() > 1)
            {
                for(auto p : _offices)
                {
                    auto wp = p.second;
                    auto sp = wp.lock();
                    if(!sp) continue;

                    auto to = meta.to.front();
                    if(to != p.first) continue;

                    message cp = m;
                    cp.meta.to.pop_front();
                    cp.meta.from.push_front(_address);

                    if(sp->send(cp)) return true;
                }

                //if no parent, 
                //try to send message to outside world
                if(!_parent) return send_outside(m);

                //otherwise send to parent.
                //the post office address is added here
                //to the from so that the recieve can send message
                //back to sender
                m.meta.from.push_front(_address);
                return _parent->send(m);
            }

            //route to mailbox
            CHECK_EQUAL(meta.to.size(), 1);

            for(auto p : _boxes)
            {
                auto wb = p.second;
                auto sb = wb.lock();
                if(!sb) continue;

                auto to = meta.to.front();
                if(to != p.first) continue;

                sb->push_inbox(m);
                return true;
            }

            //could not send message
            return false;
        }

        bool post_office::add(mailbox_wptr p)
        {
            auto sp = p.lock();
            if(!sp) return false;

            REQUIRE_FALSE(sp->address().empty());
            REQUIRE_FALSE(_boxes.count(sp->address()));

            _boxes[sp->address()] = p;

            return true;
        }

        void post_office::remove_mailbox(const std::string& n)
        {
            _boxes.erase(n);
        }

        bool post_office::add(post_office_wptr p)
        {
            auto sp = p.lock();
            if(!sp) return false;

            REQUIRE_FALSE(sp->address().empty());
            REQUIRE_FALSE(_offices.count(sp->address()));

            sp->parent(this);
            _offices[sp->address()] = p;

            return true;
        }

        void post_office::remove_post_office(const std::string& n)
        {
            _offices.erase(n);
        }

        post_office* post_office::parent() 
        { 
            return _parent; 
        }

        const post_office* post_office::parent() const 
        { 
            return _parent; 
        }

        void post_office::parent(post_office* p) 
        { 
            REQUIRE(p);
            _parent = p;
        }
        
        bool post_office::send_outside(const message& m)
        {
            std::cerr <<"TODO: send_outside" << std::endl;
            std::cerr << "to: " << m.meta.to << std::endl;
            std::cerr << "from: " << m.meta.from << std::endl;
            std::cerr << "\t" << m << std::endl;
        }
    }
}
