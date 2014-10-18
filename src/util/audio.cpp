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
#ifndef FIRESTR_UTIL_AUDIO_H
#define FIRESTR_UTIL_AUDIO_H

#include "util/audio.hpp"
#include "util/dbc.hpp"
#include "util/log.hpp"

namespace u = fire::util;

namespace fire
{
    namespace util
    {
        const size_t FRAMES = 480; //40ms of PCM frames. Opus can handles 2.5, 5, 10, 20, 40 or 60ms of audio per frame.
        const size_t MAX_FRAMES = 2*FRAMES;
        const size_t MAX_OPUS_DECODE_SIZE = MAX_FRAMES * sizeof(opus_int16);
        const size_t SAMPLE_RATE = 12000;
        const size_t CHANNELS = 1;
        const size_t MIN_BUF_SIZE = FRAMES * sizeof(opus_int16);

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

        bytes opus_encoder::encode(const bytes& b)
        {
            REQUIRE(_opus);
            REQUIRE_FALSE(b.empty());
            if(b.size() != MIN_BUF_SIZE) return {};

            u::bytes r;
            r.resize(MIN_BUF_SIZE);
            auto size = opus_encode(_opus, 
                    reinterpret_cast<const opus_int16*>(b.data()),
                    FRAMES,
                    reinterpret_cast<unsigned char*>(r.data()),
                    r.size());

            if(size < 0) 
            {
                LOG << "opus encode error: "; log_opus_error(size);
                return {};
            }
            r.resize(size);
            return r;
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

        bytes opus_decoder::decode(const bytes& b)
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
            return t;
        }
    }
}

#endif
