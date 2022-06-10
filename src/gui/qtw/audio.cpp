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

#include <QtWidgets>
#include <QMediaDevices>

#include "gui/qtw/audio.hpp"
#include "gui/qtw/frontend.hpp"
#include "gui/util.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace u = fire::util;

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            namespace 
            {
                const size_t FRAMES = u::FRAMES; //40ms of PCM frames. Opus can handles 2.5, 5, 10, 20, 40 or 60ms of audio per frame.
                const size_t SAMPLE_RATE = u::SAMPLE_RATE;
                const size_t SAMPLE_SIZE = 16;
                const size_t MAX_SAMPLE_BYTES = SAMPLE_SIZE * FRAMES;
                const size_t CHANNELS = u::CHANNELS;
                const std::string Q_CODEC = "audio/pcm";
                const size_t MIN_BUF_SIZE = u::MIN_BUF_SIZE;
            }

            codec_type parse_codec(const std::string& codec)
            {
                if(codec == "pcm") return codec_type::pcm;
                else if(codec == "opus") return codec_type::opus;
                return codec_type::pcm;
            }

            microphone::microphone(api::backend* back, qt_frontend* front, api::ref_id id, const std::string& codec) :  
                _id{id}, _back{back}, _front{front}
            {
                REQUIRE(back);
                REQUIRE(front);
                INVARIANT(_back);
                INVARIANT(_front);

                _f.setSampleRate(SAMPLE_RATE); 
                _f.setChannelCount(CHANNELS); 
                _f.setSampleFormat(QAudioFormat::Int16);
                //_f.setCodec(Q_CODEC.c_str()); 
                _t = parse_codec(codec);

                _inf = QMediaDevices::defaultAudioInput();
                if(_inf.isNull())
                {
                    LOG << "No audio input found" << std::endl;
                    return;
                }

                if (!_inf.isFormatSupported(_f)) 
                {
                    _f = _inf.preferredFormat();
                    LOG << "format not supported, using prefered." << std::endl;
                    LOG << "sample rate: " << _f.sampleRate() << std::endl;
                    LOG << "sample size: " << _f.bytesPerSample() << std::endl;
                }

                LOG << "using mic device: " << convert(_inf.description()) << std::endl;
                _i = new QAudioInput{_inf, _f, _front->canvas};
                TODO MAX _r = new QMediaRecorder{_front->canvas};

                CHECK_GREATER_EQUAL(static_cast<size_t>(_f.sampleRate()), SAMPLE_RATE);
                _skip = _f.sampleRate() / SAMPLE_RATE;
                _channels = _f.channelCount();

                if(_t == codec_type::opus) _opus = std::make_shared<u::opus_encoder>();
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
                size_t c = 1;
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
                if(_opus) return _opus->encode(b);
                return b;
            }

            u::bytes microphone::read_data()
            {
                if(!_i) return u::bytes{};
                if(!_d) return u::bytes{};

                auto len = _i->bytesReady();
                if(len > 0)
                {
                    if(static_cast<size_t>(len) > MAX_SAMPLE_BYTES) len = MAX_SAMPLE_BYTES;

                    u::bytes data;
                    data.resize(len);

                    auto l = _d->read(data.data(), len);
                    if(l > 0) 
                    {
                        data.resize(l);

                        //decimate and add to buffer
                        decimate(data, _buffer, _channels, _skip);
                    }
                }

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
                _recording = false;
            }

            void microphone::start()
            {
                if(!_i) return;

                INVARIANT(_back);
                if(!_d)
                {
                    _d = _i->start();
                    if(_d) _front->connect_sound(_id, _i, _d);
                }
                _recording = true;
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

            speaker::speaker(api::backend* back, qt_frontend* front, const std::string& codec) : _back{back}, _front{front}
            {
                REQUIRE(back);
                REQUIRE(front);
                INVARIANT(_back);
                INVARIANT(_front);

                _f.setSampleRate(SAMPLE_RATE); 
                _f.setChannelCount(CHANNELS); 
                _f.setSampleFormat(QAudioFormat::Int16);
                _f.setByteOrder(QAudioFormat::LittleEndian); 
                //_f.setCodec(Q_CODEC.c_str()); 
                _t = parse_codec(codec);

                auto inf = QMediaDevices::defaultAudioOutput();
                if(inf.isNull())
                {
                    LOG << "No audio output device found" << std::endl;
                    return;
                }

                if (!inf.isFormatSupported(_f)) _f = inf.nearestFormat(_f);
                LOG << "using speaker device: " << convert(inf.deviceName()) << std::endl;

                _o = new QAudioOutput{inf, _f, _front};

                CHECK_GREATER_EQUAL(static_cast<size_t>(_f.sampleRate()), SAMPLE_RATE);
                _rep = _f.sampleRate() / SAMPLE_RATE;
                _channels = _f.channelCount();

                if(_t == codec_type::opus) _opus = std::make_shared<u::opus_decoder>();
            }
            
            u::bytes speaker::decode(const u::bytes& b)
            {
                if(_opus) return _opus->decode(b);
                return b;
            }

            codec_type speaker::codec() const
            {
                return _t;
            }

            void speaker::mute()
            {
                _mute = true;
            }

            void speaker::unmute()
            {
                _mute = false;
            }

            void speaker::play(const u::bytes& d)
            {
                if(!_o) return;
                if(_mute) return;
                if(d.empty()) return;

                const u::bytes* data = &d;
                u::bytes dec;
                if(_t == codec_type::opus) 
                {
                    dec = decode(d);
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
        }
    }
}
