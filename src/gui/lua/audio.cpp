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
 *
 * In addition, as a special exception, the copyright holders give 
 * permission to link the code of portions of this program with the 
 * OpenSSL library under certain conditions as described in each 
 * individual source file, and distribute linked combinations 
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for 
 * all of the code used other than OpenSSL. If you modify file(s) with 
 * this exception, you may extend this exception to your version of the 
 * file(s), but you are not obligated to do so. If you do not wish to do 
 * so, delete this exception statement from your version. If you delete 
 * this exception statement from all source files in the program, then 
 * also delete it here.
 */

#include <QtWidgets>

#include "gui/lua/audio.hpp"
#include "gui/lua/api.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

#include <QAudioDeviceInfo>

namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace lua
        {
            namespace 
            {
                const size_t FRAMES = 480; //40ms of PCM frames. Opus can handles 2.5, 5, 10, 20, 40 or 60ms of audio per frame.
                const size_t MAX_FRAMES = 2*FRAMES;
                const size_t MAX_OPUS_DECODE_SIZE = MAX_FRAMES * sizeof(opus_int16);
                const size_t SAMPLE_RATE = 12000;
                const size_t SAMPLE_SIZE = 16;
                const size_t CHANNELS = 1;
                const std::string Q_CODEC = "audio/pcm";
                const size_t MIN_BUF_SIZE = FRAMES * sizeof(opus_int16);
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

            
            opus_encoder::opus_encoder()
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
                opus_encoder_ctl(_opus, OPUS_SET_FORCE_CHANNELS(1)); //force mono
                opus_encoder_ctl(_opus, OPUS_SET_PACKET_LOSS_PERC(2));

                ENSURE(_opus);
            }

            opus_encoder::~opus_encoder()
            {
                REQUIRE(_opus);
                opus_encoder_destroy(_opus);
            }

            bin_data opus_encoder::encode(const bin_data& b)
            {
                REQUIRE(_opus);
                REQUIRE_FALSE(b.data.empty());
                if(b.data.size() != MIN_BUF_SIZE) return {};

                u::bytes r;
                r.resize(MIN_BUF_SIZE);
                auto size = opus_encode(_opus, 
                        reinterpret_cast<const opus_int16*>(b.data.data()),
                        FRAMES,
                        reinterpret_cast<unsigned char*>(r.data()),
                        r.size());

                if(size < 0) 
                {
                    LOG << "opus encode error: "; log_opus_error(size);
                    return {};
                }
                r.resize(size);
                return {r};
            }

            opus_decoder::opus_decoder()
            {
                int err;
                _opus = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);

                if(err != OPUS_OK) 
                {
                    LOG << "opus decoder create error: "; log_opus_error(err);
                }

                ENSURE(_opus);
            }
            opus_decoder::~opus_decoder()
            {
                REQUIRE(_opus);
                opus_decoder_destroy(_opus);
            }

            bin_data opus_decoder::decode(const bin_data& b)
            {
                REQUIRE(_opus);

                u::bytes t;
                t.resize(MAX_OPUS_DECODE_SIZE);
                auto frames = opus_decode(
                        _opus,
                        reinterpret_cast<const unsigned char*>(b.data.data()),
                        b.data.size(),
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
                return {t};
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

                CHECK_GREATER_EQUAL(_f.sampleRate(), SAMPLE_RATE);
                _skip = _f.sampleRate() / SAMPLE_RATE;
                _channels = _f.channelCount();

                if(_t == codec_type::opus) _opus = std::make_shared<opus_encoder>();
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

                return _opus->encode({b}).data;
            }

            u::bytes microphone::read_data()
            {
                REQUIRE(_d);
                if(!_d) return {};

                INVARIANT(_i);

                auto len = _i->bytesReady();
                if(len == 0) return {};
                if(len > MAX_SAMPLE_BYTES) len = MAX_SAMPLE_BYTES;

                u::bytes data;
                data.resize(len);

                auto l = _d->read(data.data(), len);
                if(l <= 0) return {};
                data.resize(l);

                //decimate and add to buffer
                decimate(data, _buffer, _channels, _skip);

                if(_buffer.size() < MIN_BUF_SIZE) return {};

                //once we have enough data, do noise reduction
                reduce_noise(_buffer, MIN_BUF_SIZE);

                //copy buffer to result
                u::bytes r(_buffer.begin(), _buffer.begin() + MIN_BUF_SIZE);

                //copy extra to front and resize. Probably a better idea to use a circular buf here.
                auto final_buf_size = _buffer.size() - MIN_BUF_SIZE;
                std::copy(_buffer.begin() + MIN_BUF_SIZE, _buffer.end(), _buffer.begin());
                _buffer.resize(final_buf_size);

                return r;
            }

            codec_type microphone::codec() const
            {
                return _t;
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
                    if(_d) _api->connect_sound(_id, _i, _d);
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

                CHECK_GREATER_EQUAL(_f.sampleRate(), SAMPLE_RATE);
                _rep = _f.sampleRate() / SAMPLE_RATE;
                _channels = _f.channelCount();

                if(_t == codec_type::opus) _opus = std::make_shared<opus_decoder>();
            }
            
            u::bytes speaker::decode(const u::bytes& b)
            {
                REQUIRE(_opus);
                return _opus->decode({b}).data;
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

                //repeat to match speaker sample rate
                u::bytes r;
                inflate(*data, r, _channels, _rep);

                if(_d) 
                {
                    _d->write(r.data(), r.size());
                    if(_o->state() == QAudio::SuspendedState)
                    {
                        _o->reset();
                        _o->resume();
                    }
                }
                else
                {
                    _d = _o->start();
                    _d->write(r.data(), r.size());
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
