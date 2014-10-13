
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

#ifndef FIRESTR_UTIL_SERIALIZE_H
#define FIRESTR_UTIL_SERIALIZE_H

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include "util/mencode.hpp"

namespace fire
{
    namespace util
    {
        class mencode_out
        {
            public:
                void operator()(bool t) { _v = t; }
                void operator()(int t) { _v = t; }
                void operator()(size_t t) { _v = t; }
                void operator()(double t) { _v = t; }
                void operator()(const std::string& t) { _v = t; }
                void operator()(const bytes& t) { _v = t; }
                void operator()(const dict& t) { _v = t; }
                void operator()(const array& t) { _v = t; }
                void operator()(const value& t) { _v = t; }



            template <class C>
                void out_collection(const C& t)
                {
                    array a;
                    for(const auto& v : t)
                    {
                        mencode_out o;
                        o(v);
                        a.add(o.val());
                    }
                    _v = a;
                }

            template <class C>
                void out_collection(const std::string& k, const C& t)
                {
                    array a;
                    for(const auto& v : t)
                    {
                        mencode_out o;
                        o(v);
                        a.add(o.val());
                    }
                    _d[k] = a;
                }

            template <class M>
                void out_map(const M& t)
                {
                    dict d;
                    for(const auto& v : t)
                    {
                        mencode_out o;
                        o(v.second);
                        d[v.first] = o.val();
                    }
                    _v = d;
                }

            template <class M>
                void out_map(const std::string& k, const M& t)
                {
                    dict d;
                    for(const auto& v : t)
                    {
                        mencode_out o;
                        o(v.second);
                        d[v.first] = o.val();
                    }
                    _d[k] = d;
                }

            template <class T>
                void operator()(const std::vector<T>& t) 
                { out_collection(t); }

            template <class T>
                void operator()(const std::list<T>& t) 
                { out_collection(t); }

            template <class T>
                void operator()(const std::set<T>& t) 
                { out_collection(t); }

            template <class T>
                void operator()(const std::map<std::string, T>& t) 
                { out_map(t); }

            template <class T>
                void operator()(const std::unordered_map<std::string, T>& t) 
                { out_map(t); }

                template<class T>
                void operator()(const T& t) { 
                    T& tn = const_cast<T&>(t);
                    tn.serialize(*this);
                }

                void operator()(const std::string& k, bool t) { _d[k] = t; }
                void operator()(const std::string& k, int t) { _d[k] = t; }
                void operator()(const std::string& k, size_t t) { _d[k] = t; }
                void operator()(const std::string& k, double t) { _d[k] = t; }
                void operator()(const std::string& k, const std::string& t) { _d[k] = t; }
                void operator()(const std::string& k, const bytes& t) { _d[k] = t; }
                void operator()(const std::string& k, const dict& t) { _d[k] = t; }
                void operator()(const std::string& k, const array& t) { _d[k] = t; }
                void operator()(const std::string& k, const value& t) { _d[k] = t; }

            template <class T>
                void operator()(const std::string& k, const std::vector<T>& t) 
                { out_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, const std::list<T>& t) 
                { out_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, const std::set<T>& t) 
                { out_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, const std::map<std::string, T>& t) 
                { out_map(k, t); }

            template <class T>
                void operator()(const std::string& k, const std::unordered_map<std::string, T>& t) 
                { out_map(k, t); }

            template <class T>
                void operator()(const std::string& k, const T& t)
                {
                    mencode_out o;
                    T& tn = const_cast<T&>(t);
                    tn.serialize(o);
                    _d[k] = o.val();
                }

            value val() const { return _d.size() ? value(_d) : _v;}

            private:
                value _v;
                dict _d;
        };

        class mencode_in
        {
            public:
                mencode_in(const value& v) 
                {
                    if(v.is_dict()) _d = v.as_dict();
                    else _v = v;
                }
                mencode_in(const dict& d) : _d{d} { }

            public:
                void operator()(bool& t) {  t = _v; }
                void operator()(int& t) {  t = _v; }
                void operator()(size_t& t) {  t = _v; }
                void operator()(double& t) {  t = _v; }
                void operator()(std::string& t) {t = _v.as_string();}
                void operator()(bytes& t) {  t = _v; }
                void operator()(dict& t) {  t = _v; }
                void operator()(array& t) {  t = _v; }
                void operator()(value& t) {  t = _v; }

            template <class C>
                void in_collection(C& t)
                {
                    t.clear();
                    const auto& a = _v.as_array();
                    for(const auto& v : a)
                    {
                        typename C::value_type tv;
                        mencode_in in{v};
                        in(tv);
                        t.insert(t.end(), tv);
                    }
                }

            template <class C>
                void in_collection(const std::string& k, C& t)
                {
                    t.clear();
                    const auto& a = _d[k].as_array();
                    for(const auto& v : a)
                    {
                        typename C::value_type tv;
                        mencode_in in{v};
                        in(tv);
                        t.insert(t.end(), tv);
                    }
                }

            template <class M>
                void in_map(M& t)
                {
                    t.clear();
                    const auto& d = _v.as_dict();
                    for(const auto& v : d)
                    {
                        typename M::mapped_type tv;
                        mencode_in in{v.second};
                        in(tv);
                        t[v.first] = tv;
                    }
                }

            template <class M>
                void in_map(const std::string& k, M& t)
                {
                    t.clear();
                    const auto& d = _d[k].as_dict();
                    for(const auto& v : d)
                    {
                        typename M::mapped_type tv;
                        mencode_in in{v.second};
                        in(tv);
                        t[v.first] = tv;
                    }
                }

            template <class T>
                void operator()(std::vector<T>& t) 
                { in_collection(t); }

            template <class T>
                void operator()(std::list<T>& t) 
                { in_collection(t); }

            template <class T>
                void operator()(std::set<T>& t) 
                { in_collection(t); }

            template <class T>
                void operator()(std::map<std::string, T>& t) 
                { in_map(t); }

            template <class T>
                void operator()(std::unordered_map<std::string, T>& t) 
                { in_map(t); }

                template<class T>
                void operator()(T& t) { 
                    t.serialize(*this);
                }

                void operator()(const std::string& k, bool& t) {  t = _d[k]; }
                void operator()(const std::string& k, int& t) {  t = _d[k]; }
                void operator()(const std::string& k, size_t& t) {  t = _d[k]; }
                void operator()(const std::string& k, double& t) {  t = _d[k]; }
                void operator()(const std::string& k, std::string& t) {  t = _d[k].as_string(); }
                void operator()(const std::string& k, bytes& t) {  t = _d[k]; }
                void operator()(const std::string& k, dict& t) {  t = _d[k]; }
                void operator()(const std::string& k, array& t) {  t = _d[k]; }
                void operator()(const std::string& k, value& t) {  t = _d[k]; }

            template <class T>
                void operator()(const std::string& k, std::vector<T>& t) 
                { in_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, std::list<T>& t) 
                { in_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, std::set<T>& t) 
                { in_collection(k, t); }

            template <class T>
                void operator()(const std::string& k, std::map<std::string, T>& t) 
                { in_map(k, t); }

            template <class T>
                void operator()(const std::string& k, std::unordered_map<std::string, T>& t) 
                { in_map(k, t); }

            template <class T>
                void operator()(const std::string& k, T& t)
                {
                    mencode_in in{_d[k]};
                    t.serialize(in);
                }

            value val() const { return _d.size() ? value(_d) : _v;}
            const dict& dct() const { return _d;}

            private:
                value _v;
                dict _d;
        };

        template<class T>
            void deserialize(const bytes& b, T& t)
            {
                mencode_in in{decode<value>(b)};
                in(t);
            }

        template<class T>
            void serialize(bytes& b, const T& t)
            {
                mencode_out out;
                out(t);
                b = encode(out.val());
            }
    }
}

#define f_serialize template<class ar> void serialize(ar& a)
#define f_serialize_in void serialize(fire::util::mencode_in& a)
#define f_serialize_out void serialize(fire::util::mencode_out& a)
#define f_serialize_empty template<class ar> void serialize(ar&) {}
#define f_has(k) a.dct().has(k)
#define f_s(x) a(#x, x)
#define f_sk(k, x) a(k, x)

#endif
