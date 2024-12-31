/*
ReSonat - soft-def players react in real-time to looped echoes of sound input
by playing notes through MIDI soft-synth to sound output.

https://github.com/sunkware/resonat

Copyright (c) 2024 Sunkware

https://sunkware.org

ReSonat is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ReSonat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ReSonat. If not, see <https://www.gnu.org/licenses/>.
*/

#include "pianist.hpp"
#include "gmtimbres.hpp"
#include "../soundfonts.hpp"

Pianist::Pianist(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel) {
    this->chan = new_channel;
    new_channel++;
    fluid_synth_program_select(synth, this->chan, sfids[SFIDS::LARGE], 0, GMSS::ACOUSTIC_GRAND_PIANO);
    this->last_pitch1 = -1;
    this->last_pitch2 = -1;
    this->last_pitch3 = -1;
    this->scale = vector<int>{60, 62, 64, 65, 67, 69, 71, 72,   74, 76, 77, 79, 81, 83, 84}; // Major
}

tuple<uint8_t, uint8_t, uint8_t> Pianist::react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats) {
    tuple<uint8_t, uint8_t, uint8_t> r = {0, 0, 0};
    if ((i_blk & 0x1F) == 0) {
        int pitch = this->scale[this->scale.size() - 1 - ((this->scale.size() * spectrum_stats.argmax / (cfg::BANDWIDTH >> 2)) % this->scale.size())];
        int n = 0;
        if (spectrum_stats.max > 0xB0) {
            fluid_synth_noteoff(synth, this->chan, this->last_pitch1);
            this->last_pitch1 = pitch;
            fluid_synth_noteon(synth, this->chan, this->last_pitch1, 70);
            n++;
        }
        if (spectrum_stats.max > 0xC0) {
            fluid_synth_noteoff(synth, this->chan, this->last_pitch2);
            this->last_pitch2 = pitch + 4;
            fluid_synth_noteon(synth, this->chan, this->last_pitch2, 60);
            n++;
        }
        if (spectrum_stats.max > 0xD0) {
            fluid_synth_noteoff(synth, this->chan, this->last_pitch3);
            this->last_pitch3 = pitch + 7;
            fluid_synth_noteon(synth, this->chan, this->last_pitch3, 70);
            n++;
        }
        int c = 0x3F + (n << 6);
        r = {c, c, c};
    }
    return r;       
}
