/*
 * Copyright (C) 2017  Maxim Noah Khailo
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

#pragma once

#include "util/bytes.hpp"

#include <string>
#include <unordered_map>
#include <opus/opus.h>

namespace fire::util
{
    class opus_encoder
    {
        public:
            opus_encoder();
            ~opus_encoder();

        public:
            bytes encode(const bytes&);

        private:
            OpusEncoder* _opus = nullptr;
    };
    using opus_encoder_ptr = std::shared_ptr<opus_encoder>;

    class opus_decoder
    {
        public:
            opus_decoder();
            ~opus_decoder();

        public:
            bytes decode(const bytes&);

        private:
            OpusDecoder* _opus = nullptr;
    };
    using opus_decoder_ptr = std::shared_ptr<opus_decoder>;

    extern const size_t FRAMES; //40ms of PCM frames. Opus can handles 2.5, 5, 10, 20, 40 or 60ms of audio per frame.
    extern const size_t MAX_FRAMES;
    extern const size_t MAX_OPUS_DECODE_SIZE;
    extern const size_t SAMPLE_RATE;
    extern const size_t CHANNELS;
    extern const size_t MIN_BUF_SIZE;
}
