/*
 * Copyright (C) 2014  Maxim Noah Khailo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either vedit_refsion 3 of the License, or
 * (at your option) any later vedit_refsion.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtWidgets>

#include "gui/lua/base.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"


#include <QTimer>
#include <QAudioDeviceInfo>
#include <QSignalMapper>

#include <opus/opus.h>
#include <functional>

namespace m = fire::message;
namespace ms = fire::messages;
namespace us = fire::user;
namespace s = fire::conversation;
namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            namespace 
            {
                const std::string ARRAY_K = "__a";
                const size_t MAX_FRAMES = 3000;
                const size_t MAX_OPUS_DECODE_SIZE = MAX_FRAMES * sizeof(opus_int16);
                const size_t FRAMES_FOR_60MS = 720; //60ms of PCM frames. Opus can handles 2.5, 5, 10, 20, 40 or 60ms of audio per frame. 60ms sounds smoothest.
                const size_t MS_60_IN_MICRO = 60000;
                const size_t SAMPLE_RATE = 12000;
                const size_t SAMPLE_SIZE = 16;
                const size_t CHANNELS = 1;
                const std::string Q_CODEC = "audio/pcm";
                const size_t MIN_BUF_SIZE = FRAMES_FOR_60MS * sizeof(opus_int16);
            }

            const size_t MAX_SAMPLE_BYTES = SAMPLE_SIZE * FRAMES_FOR_60MS;

            void set_enabled(int id, widget_map& map, bool enabled)
            {
                auto w = get_widget<QWidget>(id, map);
                if(!w) return;

                w->setEnabled(enabled);
            }

            void observable_ref::set_name(const std::string& n)
            {
                std::lock_guard<std::mutex> lock(api->mutex);
                INVARIANT(api);

                auto obs = api->get_observable(id);
                if(!obs) return;

                _name = n;
                obs->_name = n;
                api->observable_names[n] = id;
            }

            std::string observable_ref::get_name() const
            {
                std::lock_guard<std::mutex> lock(api->mutex);
                return _name;
            }

            bool widget_ref::enabled()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto w = get_widget<QWidget>(id, api->widgets);
                return w ? w->isEnabled() : false;
            }

            void widget_ref::enable()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                set_enabled(id, api->widgets, true);
            }

            void widget_ref::disable()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                set_enabled(id, api->widgets, false);
            }

            std::string app_ref::get_id() const
            {
                return app_id;
            }

            void app_ref::send(const script_message& m)
            {
                INVARIANT(api);
                INVARIANT(api->sender);

                api->sender->send_to_local_app(app_id, m); 
            }

            std::string contact_ref::get_id() const
            {
                INVARIANT(api);
                return user_id;
            }

            std::string contact_ref::get_name() const
            {
                INVARIANT(api);
                INVARIANT(api->conversation);

                if(is_self) 
                    return api->conversation->user_service()->user().info().name();

                //otherwise find user
                auto c = api->conversation->contacts().by_id(user_id);
                if(!c) return "";

                return c->name();
            }

            bool contact_ref::is_online() const
            {
                INVARIANT(api);
                INVARIANT(api->conversation);

                if(is_self) return true;

                std::lock_guard<std::mutex> lock(api->mutex);
                return api->conversation->user_service()->contact_available(user_id);
            }

            size_t bin_data::get_size() const
            {
                return data.size();
            }

            char bin_data::get(size_t i) const
            {
                return i >= data.size() ? 0 : data[i];
            }

            void bin_data::set(size_t i, char c)
            {
                if(i < data.size()) data[i] = c;
            }

            std::string bin_data::to_str() const
            {
                return std::string{data.begin(), data.end()};
            }

            bin_data bin_data::sub(size_t p, size_t s) const
            {
                auto si = data.begin(); std::advance(si, p);
                auto ei = si;           std::advance(ei, s);
                return {{si, ei}};
            }

            void bin_data::append(const bin_data& n) 
            {
                data.insert(data.end(), n.data.begin(), n.data.end());
            }

            std::string bin_file_data::get_name() const
            {
                return name;
            }

            size_t bin_file_data::get_size() const
            {
                return data.data.size();
            }

            bin_data bin_file_data::get_data() const
            {
                return data;
            }
 
            bool bin_file_data::is_good() const
            {
                return good;
            }

            std::string file_data::get_name() const
            {
                return name;
            }

            size_t file_data::get_size() const
            {
                return data.size();
            }

            std::string file_data::get_data() const
            {
                return data;
            }
 
            bool file_data::is_good() const
            {
                return good;
            }

            const std::string EVENT_MESSAGE = "evt";
            event_message::event_message(
                    const std::string& obj,
                    const std::string& type,
                    const u::value& v,
                    lua_api* api) : 
                _obj{obj}, _type{type}, _v{v}, _api{api} 
                { 
                    REQUIRE(_api);
                    INVARIANT(_api);
                }

            event_message::event_message(const m::message& m, lua_api* api) :
                _api{api}
            {
                REQUIRE(api);
                REQUIRE_EQUAL(m.meta.type, EVENT_MESSAGE);

                _obj = m.meta.extra["o"].as_string();
                _type = m.meta.extra["t"].as_string();
                u::decode(m.data, _v);

                INVARIANT(_api);
            }
            
            event_message::operator m::message() const
            {
                m::message m; 
                m.meta.type = EVENT_MESSAGE;
                m.meta.extra["o"] = _obj;
                m.meta.extra["t"] = _type;
                m.data = u::encode(_v);
                return m;
            }

            const std::string SCRIPT_MESSAGE = "event_msg";
            script_message::script_message(lua_api* api) : 
                _from_id{}, _v{}, _api{api} 
                { 
                    REQUIRE(_api);
                    INVARIANT(_api);
                }

            script_message::script_message(const m::message& m, lua_api* api) :
                _api{api}
            {
                REQUIRE(api);
                REQUIRE_EQUAL(m.meta.type, SCRIPT_MESSAGE);

                _from_id = m.meta.extra["from_id"].as_string();
                if(m.meta.extra.has("t"))
                    _type = m.meta.extra["t"].as_string();
                if(m.meta.extra.has("local_app_id")) 
                    _local_app_id = m.meta.extra["local_app_id"].as_string();

                u::decode(m.data, _v);

                INVARIANT(_api);
            }
            
            script_message::operator m::message() const
            {
                m::message m; 
                m.meta.type = SCRIPT_MESSAGE;
                if(!_type.empty()) m.meta.extra["t"] = _type;
                m.data = u::encode(_v);
                m.meta.robust = _robust;
                return m;
            }

            void script_message::not_robust() 
            {
                _robust = false;
            }

            const u::value& script_message::get(const std::string& k) const
            {
                return _v[k];
            }

            void script_message::set(const std::string& k, const u::value& v) 
            {
                _v[k] = v;
            }

            bool script_message::has(const std::string& k) const 
            {
                return _v.has(k);
            }

            bin_data script_message::get_bin(const std::string& k) const
            {
                if(!_v.has(k)) return bin_data{};

                auto v = _v[k];
                if(!v.is_bytes()) return bin_data{};
                return bin_data{v.as_bytes()};
            }

            void script_message::set_bin(const std::string& k, const bin_data& v) 
            {
                _v[k] = v.data;
            }

            contact_ref empty_contact_ref(lua_api& api)
            {
                contact_ref e;
                e.api = &api;
                e.id = 0;
                e.user_id = "0";
                return e;
            }

            contact_ref script_message::from() const
            {
                INVARIANT(_api);
                INVARIANT(_api->conversation);
                std::lock_guard<std::mutex> lock(_api->mutex);

                auto c = _api->conversation->contacts().by_id(_from_id);
                if(!c || is_local()) return empty_contact_ref(*_api);

                contact_ref r;
                r.id = 0;
                r.user_id = c->id();
                r.api = _api;

                ENSURE_EQUAL(r.api, _api);
                ENSURE_FALSE(r.user_id.empty());
                return r;
            }

            bool script_message::is_local() const
            {
                return !_local_app_id.empty();
            }

            app_ref empty_app_ref(lua_api& api)
            {
                app_ref e;
                e.api = &api;
                e.id = 0;
                e.app_id = "0";
                return e;
            }

            app_ref script_message::app() const
            {
                INVARIANT(_api);

                if(!is_local()) return empty_app_ref(*_api);

                app_ref r;
                r.id = 0;
                r.app_id = _local_app_id;
                r.api = _api;

                ENSURE_EQUAL(r.api, _api);
                ENSURE_FALSE(r.app_id.empty());
                return r;
            }

            void script_message::set_type(const std::string& t)
            {
                _type = t;
            }

            const std::string& script_message::get_type() const
            {
                return _type;
            }


            store_ref::store_ref(u::disk_store& d) : _d(d){}

            std::string store_ref::get(const std::string& k) const
            {
                if(!_d.has(k)) return "";
                return _d.get(k).as_string();
            }

            void store_ref::set(const std::string& k, const std::string& v) 
            {
                _d.set(k, v);
            }

            bin_data store_ref::get_bin(const std::string& k) const

            {
                if(!_d.has(k)) return bin_data{};

                auto v = _d.get(k);
                if(!v.is_bytes()) return bin_data{};
                return bin_data{v.as_bytes()};
            }

            void store_ref::set_bin(const std::string& k, const bin_data& v) 
            {
                _d.set(k,v.data);
            }

            bool store_ref::has(const std::string& k) const
            {
                return _d.has(k);
            }

            bool store_ref::remove(const std::string& k)
            {
                return _d.remove(k);
            }

            size_t store_ref::size() const
            {
                return _d.size();
            }

            const util::disk_store& store_ref::store() const
            {
                return _d;
            }

            util::disk_store& store_ref::store()
            {
                return _d;
            }

            u::dict to_dict(lua_State* L, int table)
            {
                REQUIRE(L);

                u::dict d;
                //push first key
                lua_pushnil(L);
                while (lua_next(L, table) != 0)
                {
                    //extract key
                    std::string key;
                    int ik = -1;
                    if(lua_isnumber(L, -2)) ik = lua_tonumber(L, -2); 
                    else key = lua_tostring(L, -2);

                    //extract value
                    u::value v;
                    if(lua_isnumber(L, -1)) v = lua_tonumber(L, -1);
                    else if(lua_isstring(L, -1)) v = std::string{lua_tostring(L, -1)};
                    else if(lua_istable(L, -1)) v = to_dict(L, lua_gettop(L));
                    else 
                    {
                        lua_pop(L, 1);
                        continue;
                    }

                    //insert to dict
                    if(ik != -1)
                    {
                        u::array* a = nullptr;
                        if(!d.has(ARRAY_K))
                        {
                            u::array na;
                            d[ARRAY_K] = na;
                            a = &d[ARRAY_K].as_array();
                        } 
                        else a = &d[ARRAY_K].as_array();

                        CHECK(a);

                        //lua arrays start with 1
                        if(ik != 0) ik --;
                        if(ik >= a->size()) a->resize(ik + 1);

                        (*a)[ik] = v;
                    }
                    else d[key] = v;

                    lua_pop(L, 1);
                }
                return d;
            }

            void push_dict(lua_State* L, const u::dict& d);
            void push_array(lua_State* L, const u::array& a);
            void push_value(lua_State* L, const u::value& v)
            {
                REQUIRE(L);

                if(v.is_double())
                    lua_pushnumber(L, v.as_double());
                else if(v.is_bytes())
                    lua_pushstring(L, v.as_string().c_str());
                else if(v.is_dict())
                    push_dict(L, v.as_dict());
                else if(v.is_array())
                    push_array(L, v.as_array());
            }

            void push_array(lua_State* L, const u::array& a, int table)
            {
                REQUIRE(L);

                int i = 1;
                for(const auto& v : a)
                {
                    lua_pushnumber(L, i);
                    push_value(L, v);
                    lua_settable(L, table);
                    i++;
                }
            }

            void push_array(lua_State* L, const u::array& a)
            {
                REQUIRE(L);

                lua_newtable(L);
                auto table = lua_gettop(L);
                push_array(L, a, table);
            }


            void push_dict(lua_State* L, const u::dict& d)
            {
                REQUIRE(L);

                lua_newtable(L);
                auto table = lua_gettop(L);
                for(const auto& p : d)
                {
                    if(p.first == ARRAY_K) push_array(L, p.second, table);
                    else
                    {
                        lua_pushstring(L, p.first.c_str());
                        push_value(L, p.second);
                        lua_settable(L, table);
                    }
                }
            }

            u::value to_value(lua_State* L, int param)
            {
                REQUIRE(L);

                u::value v;
                if(lua_istable(L, param)) v = to_dict(L, param);
                else if(lua_isnumber(L, param)) v = lua_tonumber(L, param);
                else if(lua_isstring(L, param)) v = std::string{lua_tostring(L, param)};
                else if(lua_isboolean(L, param)) v = lua_toboolean(L, param) ? 1 : 0;
                return v;
            }

            int store_ref_set(lua_State* L)
            {
                REQUIRE(L);

                auto params = lua_gettop(L);
                if(params != 3) return 0;

                //get store ref
                store_ref* r = SLB::Private::Type<store_ref*>::get(L, 1);
                if(r == nullptr) return 0;
                if(!lua_isstring(L, 2)) return 0;

                //get key
                std::string key = lua_tostring(L, 2);
                if(key.empty()) return 0;

                //set value
                u::value v = to_value(L, 3);
                r->store().set(key, v);

                return 0;
            }

            int store_ref_get(lua_State* L)
            {
                REQUIRE(L);

                auto params = lua_gettop(L);
                if(params != 2) { lua_pushnil(L); return 1; }

                //get store ref
                store_ref* r = SLB::Private::Type<store_ref*>::get(L, 1);
                if(!r) { lua_pushnil(L); return 1; }
                if(!lua_isstring(L, 2)) { lua_pushnil(L); return 1; }

                //get key
                std::string key = lua_tostring(L, 2);
                if(!r->store().has(key)) { lua_pushnil(L); return 1;} 

                //return value
                auto v = r->store().get(key);
                push_value(L, v);
                return 1;
            }

            int script_message_set(lua_State* L)
            {
                REQUIRE(L);
                auto params = lua_gettop(L);
                if(params != 3) return 0;

                //get script message
                script_message* r = SLB::Private::Type<script_message*>::get(L, 1);
                if(r == nullptr) return 0;
                if(!lua_isstring(L, 2)) return 0;

                //get key
                std::string key = lua_tostring(L, 2);
                if(key.empty()) return 0;

                //set value
                u::value v = to_value(L, 3);
                r->set(key, v);

                return 0;
            }

            int script_message_get(lua_State* L)
            {
                auto params = lua_gettop(L);
                if(params != 2) { lua_pushnil(L); return 1; }

                //get script message
                script_message* r = SLB::Private::Type<script_message*>::get(L, 1);
                if(!r) { lua_pushnil(L); return 1; }
                if(!lua_isstring(L, 2)) { lua_pushnil(L); return 1; }

                //get key
                std::string key = lua_tostring(L, 2);
                if(!r->has(key)) { lua_pushnil(L); return 1;} 

                //return value
                auto v = r->get(key);
                push_value(L, v);
                return 1;
            }

            codec_type parse_codec(const std::string& codec)
            {
                if(codec == "pcm") return codec_type::pcm;
                else if(codec == "opus") return codec_type::opus;
                return codec_type::pcm;
            }

            void log_opus_error(int e)
            {
                switch(e)
                {
                    case OPUS_ALLOC_FAIL: LOG << "BAD ALLOC" << std::endl; break;
                    case OPUS_BAD_ARG: LOG << "BAD ARG" << std::endl; break;
                    case OPUS_BUFFER_TOO_SMALL: LOG << "TOO SMALL" << std::endl; break;
                    case OPUS_INTERNAL_ERROR: LOG << "INTERNAL ERR" << std::endl; break;
                    case OPUS_INVALID_PACKET: LOG << "INVALID PACKET" << std::endl; break;
                    case OPUS_INVALID_STATE: LOG << "INVALID STATE" << std::endl; break;
                    case OPUS_OK: LOG << "OK" << std::endl; break;
                    default: LOG << e << std::endl;
                }
            }

            microphone::microphone(lua_api* api, int id, const std::string& codec) : _api{api}, _id{id}
            {
                REQUIRE(api);
                INVARIANT(_api);

                _f.setSampleRate(SAMPLE_RATE); 
                _f.setChannelCount(CHANNELS); 
                _f.setSampleSize(SAMPLE_SIZE); 
                _f.setSampleType(QAudioFormat::SignedInt); 
                _f.setByteOrder(QAudioFormat::LittleEndian); 
                _f.setCodec(Q_CODEC.c_str()); 
                _t = parse_codec(codec);

                _inf = QAudioDeviceInfo::defaultInputDevice();
                if (!_inf.isFormatSupported(_f)) 
                {
                    _f = _inf.nearestFormat(_f);
                    LOG << "format not supported, using nearest." << std::endl;
                    LOG << "sample rate: " << _f.sampleRate() << std::endl;
                    LOG << "sample size: " << _f.sampleSize() << std::endl;
                    LOG << "channels: " << _f.channelCount() << std::endl;
                    LOG << "codec: " << convert(_f.codec()) << std::endl;
                }

                LOG << "using mic device: " << convert(_inf.deviceName()) << std::endl;
                _i = new QAudioInput{_inf, _f, _api->canvas};

                if(_t == codec_type::opus)
                {
                    int err;

                    _opus = opus_encoder_create(SAMPLE_RATE,CHANNELS, OPUS_APPLICATION_VOIP, &err); 

                    if(err != OPUS_OK) 
                    { 
                        LOG << "opus encoder create error: "; 
                        log_opus_error(err);
                    }

                    opus_encoder_ctl(_opus, OPUS_SET_BITRATE(OPUS_AUTO));
                    opus_encoder_ctl(_opus, OPUS_SET_VBR(1));

                    _skip = _f.sampleRate() / SAMPLE_RATE;
                    _channels = _f.channelCount();
                }
            }

            microphone::~microphone()
            {
                if(_opus) opus_encoder_destroy(_opus);
            }

            //simple low pass filter 
            void reduce_noise(u::bytes& s, size_t len)
            {
                REQUIRE_EQUAL(s.size() % 2, 0);
                auto ss = reinterpret_cast<short*>(s.data());
                len /= 2;
                for(size_t i = 1; i < len; i++)
                    ss[i] = (0.333 * ss[i]) + ((1 - 0.333) * ss[i-1]) + 0.5;
            }

            //decimate sound to SAMPLE_RATE, using averaging
            void decimate(const u::bytes& s, u::bytes& d, size_t channels, size_t skip) 
            {
                REQUIRE_FALSE(s.empty());
                REQUIRE_EQUAL(s.size() % 2, 0);

                //get sizes
                auto dz = (s.size() / skip);
                auto nz = d.size() + dz;

                //add padding
                if(nz % 2 == 1) nz += 1;
                CHECK_EQUAL(nz % 2, 0);

                //resize dest
                const auto odz = d.size();
                d.resize(nz);

                //cast to short arrays
                auto ss = reinterpret_cast<const short*>(s.data());
                const auto sz = s.size() / 2;
                auto sd = reinterpret_cast<short*>(d.data());
                const auto sdz = nz / 2;

                int accum = 0;
                int c = 1;
                size_t si = 0;
                auto di = odz / 2;

                for(;si < sz; si+=channels)
                {
                    accum += static_cast<int>(ss[si]);
                    if(c == skip)
                    {
                        accum /= c;
                        sd[di] = accum;
                        di++;

                        accum = 0;
                        c = 1;
                        continue;
                    }
                    c++;
                }

                //repeat last value if we have padding
                si = sz-1;
                while(di < sdz)
                {
                    sd[di] = ss[si];
                    di++;
                }

                CHECK_EQUAL(di, sdz);
            }

            u::bytes microphone::encode(const u::bytes& b)
            {
                REQUIRE(_opus);
                REQUIRE_FALSE(b.empty());

                //add to buffer
                //decimate mic to match SAMPLE_RATE of opus encoder
                decimate(b, _buffer, _channels, _skip);

                //only encode if size of buffer is greater or equal to 60ms of audio 
                u::bytes r;
                if(_buffer.size() >= MIN_BUF_SIZE)
                {
                    reduce_noise(_buffer, MIN_BUF_SIZE);
                    r.resize(MIN_BUF_SIZE);
                    auto size = opus_encode(_opus, 
                            reinterpret_cast<const opus_int16*>(_buffer.data()),
                            FRAMES_FOR_60MS,
                            reinterpret_cast<unsigned char*>(r.data()),
                            r.size());

                    if(size < 0) 
                    {
                        LOG << "opus encode error: "; log_opus_error(size);
                        return {};
                    }
                    r.resize(size);
                }

                if(_buffer.size() > MIN_BUF_SIZE)
                {
                    auto final_buf_size = _buffer.size() - MIN_BUF_SIZE;
                    std::copy(_buffer.begin() + MIN_BUF_SIZE, _buffer.end(), _buffer.begin());
                    _buffer.resize(final_buf_size);
                }

                return r;
            }

            codec_type microphone::codec() const
            {
                return _t;
            }

            QAudioInput* microphone::input()
            {
                ENSURE(_i);
                return _i;
            }

            QIODevice* microphone::io()
            {
                REQUIRE(_d);
                return _d;
            }

            bool microphone::recording() const
            {
                return _recording;
            }

            void microphone::stop()
            {
                INVARIANT(_i);
                _recording = false;
            }

            void microphone::start()
            {
                INVARIANT(_i);
                INVARIANT(_api);
                if(!_d)
                {
                    _d = _i->start();
                    _api->connect_sound(_id, _i, _d);
                }
                _recording = true;
            }

            void microphone_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto mp = api->mic_refs.find(id);
                if(mp == api->mic_refs.end()) return;

                mp->second.callback = c;
                callback = c;
            }

            void microphone_ref::stop()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto mp = api->mic_refs.find(id);
                if(mp == api->mic_refs.end()) return;

                CHECK(mp->second.mic);
                mp->second.mic->stop();
            }

            void microphone_ref::start()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto mp = api->mic_refs.find(id);
                if(mp == api->mic_refs.end()) return;

                CHECK(mp->second.mic);
                mp->second.mic->start();
            }

            void inflate(const u::bytes& s, u::bytes& d, size_t channels, size_t rep)
            {
                REQUIRE_EQUAL(s.size() % 2, 0);

                rep*=channels;
                d.resize(s.size() * rep);

                auto ss = reinterpret_cast<const short*>(s.data());
                auto sz = s.size() / 2;
                auto sd = reinterpret_cast<short*>(d.data());

                size_t di = 0;
                for(size_t si = 0; si < sz; si++)
                    for(size_t p = 0; p < rep; p++, di++)
                        sd[di] = ss[si];
            }

            speaker::speaker(lua_api* api, const std::string& codec) : _api{api}
            {
                REQUIRE(api);
                INVARIANT(_api);

                _f.setSampleRate(SAMPLE_RATE); 
                _f.setChannelCount(CHANNELS); 
                _f.setSampleSize(SAMPLE_SIZE); 
                _f.setSampleType(QAudioFormat::SignedInt); 
                _f.setByteOrder(QAudioFormat::LittleEndian); 
                _f.setCodec(Q_CODEC.c_str()); 
                _t = parse_codec(codec);

                QAudioDeviceInfo i{QAudioDeviceInfo::defaultOutputDevice()};
                if (!i.isFormatSupported(_f)) _f = i.nearestFormat(_f);
                LOG << "using speaker device: " << convert(i.deviceName()) << std::endl;
                _o = new QAudioOutput{i, _f, _api};

                if(_t == codec_type::opus)
                {
                    int err;
                    _opus = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);

                    if(err != OPUS_OK) 
                    {
                        LOG << "opus decoder create error: "; log_opus_error(err);
                    }

                    opus_decoder_ctl(_opus, OPUS_SET_BITRATE(OPUS_AUTO));
                    opus_decoder_ctl(_opus, OPUS_SET_VBR(1));

                    _rep = _f.sampleRate() / SAMPLE_RATE;
                    _channels = _f.channelCount();
                }
            }
            
            speaker::~speaker()
            {
                if(_opus) opus_decoder_destroy(_opus);
            }

            u::bytes speaker::decode(const u::bytes& b)
            {
                REQUIRE(_opus);

                u::bytes t;
                t.resize(MAX_OPUS_DECODE_SIZE);
                auto frames = opus_decode(
                        _opus,
                        reinterpret_cast<const unsigned char*>(b.data()),
                        b.size(),
                        reinterpret_cast<opus_int16*>(t.data()),
                        MAX_FRAMES,
                        0);

                if(frames < 0) 
                {
                    LOG << "opus error decoding: "; log_opus_error(frames);
                    return {};
                }

                auto size = frames * sizeof(opus_int16);
                t.resize(size);

                //repeat to match speaker sample rate
                u::bytes r;
                inflate(t, r, _channels, _rep);

                return r;
            }

            codec_type speaker::codec() const
            {
                return _t;
            }

            void speaker::mute()
            {
                INVARIANT(_o);
                _mute = true;
            }

            void speaker::unmute()
            {
                INVARIANT(_o);
                _mute = false;
            }

            void speaker::play(const bin_data& d)
            {
                INVARIANT(_o);
                if(_mute) return;
                if(d.data.empty()) return;

                const u::bytes* data = &d.data;
                u::bytes dec;
                if(_t == codec_type::opus) 
                {
                    dec = decode(d.data);
                    data = &dec;
                }

                CHECK(data);

                if(_d) 
                {
                    _d->write(data->data(), data->size());
                    if(_o->state() == QAudio::SuspendedState)
                    {
                        _o->reset();
                        _o->resume();
                    }
                }
                else
                {
                    _d = _o->start();
                    _d->write(data->data(), data->size());
                }
            }

            void speaker_ref::mute()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto sp = api->speaker_refs.find(id);
                if(sp == api->speaker_refs.end()) return;

                CHECK(sp->second.spkr);
                sp->second.spkr->mute();
            }

            void speaker_ref::unmute()
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto sp = api->speaker_refs.find(id);
                if(sp == api->speaker_refs.end()) return;

                CHECK(sp->second.spkr);
                sp->second.spkr->unmute();
            }

            void speaker_ref::play(const bin_data& d)
            {
                INVARIANT(api);
                std::lock_guard<std::mutex> lock(api->mutex);

                auto sp = api->speaker_refs.find(id);
                if(sp == api->speaker_refs.end()) return;

                CHECK(sp->second.spkr);
                sp->second.spkr->play(d);
            }
        }
    }
}
