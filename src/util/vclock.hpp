/*
 * Copyright (C) 2017  Maxim Noah Khailo
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
#pragma once

#include <map>
#include <set>
#include <algorithm>
#include <iterator>
#include <ostream>
#include <memory>
#include "util/dbc.hpp"
#include "util/mencode.hpp"

namespace fire::util
{
    namespace detail
    {
        template <class m>
            auto get_keys(const m& p) -> std::set<typename m::key_type>
            {
                using kt = typename m::key_type;
                std::set<kt> r;
                std::transform(std::begin(p), std::end(p), 
                        std::inserter(r, r.end()),
                        [](const typename m::value_type& p) { return p.first;});
                return r;
            }

        template <class ks>
            auto merge_keys(const ks& k1, const ks& k2) -> std::set<typename ks::key_type>
            {
                ks r;
                std::set_union(
                        std::begin(k1), std::end(k1), 
                        std::begin(k2), std::end(k2),
                        std::inserter(r, r.end()));
                return r;
            }
    }

    /**
     * Simple implementation of a vector clock/version vector with compare, increment, and merge.
     * A vector clock can be used to create a partial order of events or determining when two messages
     * happened concurrently. 
     *
     * It keeps track of what 'last seen' or 'last version' count for each 
     * client who receives a message and provides semantics for incrementing and merging vector clocks.
     */
    template <class id_t>
        class vclock
        {
            public:
                using id_type = id_t;
                using clock_vec = std::map<id_t, size_t>;
                using iterator = typename clock_vec::iterator;
                using const_iterator = typename clock_vec::const_iterator;

            public:

                size_t& operator[](id_t i) { return _c[i];}

                /**
                 * The id is expected to be there if you don't want to change it.
                 */
                size_t operator[](id_t i) const 
                { 
                    REQUIRE_GREATER(i, 0);

                    auto it = _c.find(i);
                    CHECK(it != _c.end());
                    return it->second;
                }

                iterator begin() { return std::begin(_c);}
                iterator end() { return std::end(_c);}

                const_iterator begin() const { return std::begin(_c);}
                const_iterator end() const { return std::end(_c);}

                bool has(id_t i) const { return _c.count(i);}
                bool empty() const { return _c.empty();}

                /**
                 * Merge clock
                 */
                vclock& operator += (const vclock& o) 
                {
                    /**
                     * For each key in the other take the max. 
                     * If the key didn't exist in this clock,
                     * then it will be added by the [] operator.
                     */
                    for(const auto& p : o._c)
                        _c[p.first] = std::max(_c[p.first], p.second);

                    return *this;
                }

                /**
                 * Returns merged clock without modifying
                 */
                vclock operator + (const vclock& o)
                {
                    vclock n = *this;
                    n+= o;
                    return n;
                }

                /**
                 * returns -1 if o is greater
                 * returns 0 may be identical, or may not be
                 * returns 1 if o is less
                 */
                int compare(const vclock& o) const
                {
                    bool l = false;
                    bool g = false;

                    const auto keys = detail::merge_keys(detail::get_keys(_c), detail::get_keys(o._c));

                    for(const auto& k : keys)
                    {
                        const auto lvi = _c.find(k); 
                        const auto rvi = o._c.find(k);

                        const size_t lv = lvi != std::end(_c) ? lvi->second : 0;
                        const size_t rv = rvi != std::end(o._c) ? rvi->second : 0;

                        const auto diff = static_cast<ptrdiff_t>(lv) - static_cast<ptrdiff_t>(rv);

                        if(diff > 0) g = true;
                        if(diff < 0) l = true;
                    }

                    if(l && g) return 0;
                    if(l) return -1;
                    if(g) return 1;
                    return 0;
                }

                bool operator == (const vclock& o) const { return identical(o); }

                bool operator != (const vclock& o) const { return !identical(o); }

                bool operator < (const vclock& o) const { return compare(o) == -1; }

                bool operator > (const vclock& o) const { return compare(o) == 1; }

                bool operator <= (const vclock& o) const { return compare(o) == -1 || identical(o); }

                bool operator >= (const vclock& o) const { return compare(o) == 1 || identical(o); }

                bool identical(const vclock& o) const { return _c == o._c; }

                bool concurrent(const vclock& o) const { return compare(o) == 0; }

                bool conflict(const vclock& o) { return concurrent(o) && !identical(o); }

                const clock_vec& clocks() const { return _c; }

            private:
                clock_vec _c;
        };

    /**
     * A tracked vector clock/version vector associates the vclock with an id.
     * Typically the id will represent a node in the network.
     *
     * This assumption helps simplify the vclock interface
     */
    template <class id_t>
        class tracked_vclock
        {
            public:
                using id_type = id_t;
                using iterator = typename vclock<id_t>::iterator;
                using const_iterator = typename vclock<id_t>::const_iterator;

            public:

                /**
                 * New clock using id
                 */
                tracked_vclock(id_t i) : _c{}, _i(i) { _c[i] = 1;}
                tracked_vclock(id_t i, size_t v) : _c{}, _i(i) { _c[i] = v;}
                tracked_vclock(id_t i, const vclock<id_t>& c) : _c(c), _i(i) 
            {
                if(!_c.has(i)) _c[i] = 1;
            }

                operator vclock<id_t>() const { return _c;}

            public:

                /**
                 * Increment clock
                 */
                tracked_vclock& operator ++ () {_c[_i]++;};
                tracked_vclock operator ++ (int) { tracked_vclock o{*this}; _c[_i]++; return o;}

                /**
                 * Merge tracked clock. 
                 */
                tracked_vclock& operator += (const tracked_vclock& o) 
                {
                    REQUIRE_FALSE(o._c.empty());

                    //merge clock
                    _c += o._c;

                    INVARIANT(_c.has(_i));
                    return *this;
                }

                /**
                 * returns -1 if o is greater
                 * returns 0 may be identical, or may not be
                 * returns 1 if o is less
                 */
                int compare(const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));

                    return _c.compare(o._c);
                }

                tracked_vclock& operator = (const size_t v) 
                {
                    INVARIANT(_c.has(_i));
                    _c[_i] = v;
                    return *this;
                }

                size_t& operator[](id_t i) { return _c[i];}
                const size_t& operator[](id_t i) const { return _c[i];}

                bool operator == (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c.identical(o._c);
                }

                bool operator != (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return !_c.identical(o._c);
                }

                bool operator < (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c < o._c;
                }

                bool operator > (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c > o._c;
                }

                bool operator <= (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c <= o._c;
                }

                bool operator >= (const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c >= o._c;
                }

                bool identical(const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c.identical(o._c);
                }

                bool concurrent(const tracked_vclock& o) const
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c.concurrent(o._c);
                }

                bool conflict(const tracked_vclock& o)
                {
                    REQUIRE_FALSE(o._c.empty());
                    INVARIANT(_c.has(_i));
                    return _c.conflict(o._c);
                }

                id_t id() const { return _i;}
                const vclock<id_t>& clock() const { return _c;}
                vclock<id_t>& clock() { return _c;}

            private:
                vclock<id_t> _c;
                id_t _i;
        };

    /**
     * A vclock where Ids are strings. This can be used to store
     * clocks based on contact ids.
     */
    using sclock = vclock<std::string>;
    using sclock_ptr = std::shared_ptr<sclock>;
    using tracked_sclock = tracked_vclock<std::string>;
    using tracked_sclock_ptr = std::shared_ptr<tracked_sclock>;

    dict to_dict(const sclock&);
    sclock to_sclock(const dict&);

    dict to_dict(const tracked_sclock&);
    tracked_sclock to_tracked_sclock(const dict&);

    template <class id_t> 
        std::ostream& operator << (std::ostream& o, const vclock<id_t>& v)
        {
            o << "[ ";
            for(const auto& c : v)
                o << c.first << "=>" << c.second << " "; 
            o << "]";
            return o;
        }
    template <class id_t> 
        std::ostream& operator << (std::ostream& o, const tracked_vclock<id_t>& v)
        {
            o << v.id() << ": " << v.clock();
            return o;
        }
} 
