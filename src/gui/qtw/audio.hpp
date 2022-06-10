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

#ifndef FIRESTR_GUI_QTW_AUDIO_H
#define FIRESTR_GUI_QTW_AUDIO_H

#include "gui/api/service.hpp"
#include "util/audio.hpp"

#include <QAudioFormat>
#include <QAudioInput>
#include <QMediaRecorder>
#include <QAudioOutput>
#include <QAudioDevice>

#include <string>
#include <unordered_map>
#include <opus/opus.h>

namespace fire
{
    namespace gui
    {
        namespace qtw
        {
            class qt_frontend;

            enum codec_type { pcm, opus};
            class microphone
            {
                public:
                    microphone(api::backend*, qt_frontend*, api::ref_id id, const std::string& codec = "pcm");
                    void stop();
                    void start();
                    bool recording() const;
                    codec_type codec() const;
                    util::bytes encode(const util::bytes&);
                    util::bytes read_data();

                private:
                    QAudioFormat _f;
                    QAudioDevice _inf;
                    QAudioInput* _i = nullptr;
                    QMediaRecorder* _r = nullptr;
                    codec_type _t;
                    api::ref_id _id;
                    QIODevice* _d = nullptr;
                    bool _recording = false;
                    api::backend* _back;
                    qt_frontend* _front;

                    //opus specific members
                    util::opus_encoder_ptr _opus;
                    util::bytes _buffer;
                    size_t _skip = 0;
                    size_t _channels = 0;
            };
            using microphone_ptr = std::shared_ptr<microphone>;

            struct speaker 
            {
                public:
                    speaker(api::backend*, qt_frontend*, const std::string& code = "pcm");
                    void mute();
                    void unmute();
                    void play(const util::bytes&);
                    codec_type codec() const;
                    util::bytes decode(const util::bytes&);
                private:
                    bool _mute = false;
                    QAudioFormat _f;
                    codec_type _t;
                    QAudioOutput* _o;
                    QIODevice* _d = nullptr;
                    api::backend* _back;
                    qt_frontend* _front;

                    //opus specific members
                    util::opus_decoder_ptr _opus;
                    size_t _rep = 0;
                    size_t _channels = 0;
            };
            using speaker_ptr = std::shared_ptr<speaker>;
        }
    }
}

#endif
