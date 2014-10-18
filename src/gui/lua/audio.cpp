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
            void microphone_ref::set_callback(const std::string& c)
            {
                INVARIANT(api);

                auto mp = api->mic_refs.find(id);
                if(mp == api->mic_refs.end()) return;

                mp->second.callback = c;
                callback = c;
            }

            void microphone_ref::stop()
            {
                INVARIANT(api);
                INVARIANT(api->front);
                api->front->mic_stop(id);
            }

            void microphone_ref::start()
            {
                INVARIANT(api);
                INVARIANT(api->front);
                api->front->mic_start(id);
            }

            void speaker_ref::mute()
            {
                INVARIANT(api);
                INVARIANT(api->front);
                api->front->speaker_mute(id);
            }

            void speaker_ref::unmute()
            {
                INVARIANT(api);
                INVARIANT(api->front);
                api->front->speaker_unmute(id);
            }

            void speaker_ref::play(const bin_data& d)
            {
                INVARIANT(api);
                api->front->speaker_play(id, d.data);
            }
        }
    }
}
