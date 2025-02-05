/*
ReSonat - soft-def players react in real-time to looped echoes of sound input
by playing notes through MIDI soft-synth to sound output.

https://github.com/sunkware/resonat

Copyright (c) 2024-2025 Sunkware

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

#include "flutist.hpp"
#include "gmtimbres.hpp"
#include "scales.hpp"
#include "../soundfonts.hpp"

Flutist::Flutist(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel) {
    this->chan = new_channel;
    new_channel++;
    fluid_synth_program_select(synth, this->chan, sfids[SFIDS::SMALL], 0, GMSS::PAN_FLUTE);
    this->last_pitch = -1;
}

tuple<uint8_t, uint8_t, uint8_t> Flutist::react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats) {
    tuple<uint8_t, uint8_t, uint8_t> r = {0, 0, 0};
    if ((i_blk & 0x3F) == 0x30) {
        if (spectrum_stats.max > 0x80) {
            int pitch = this->scale[(this->scale.size() * spectrum_stats.argmax / (cfg::BANDWIDTH >> 2)) % this->scale.size()];
            if (pitch != this->last_pitch) {
                fluid_synth_noteoff(synth, this->chan, this->last_pitch);
                this->last_pitch = pitch;
                fluid_synth_noteon(synth, this->chan, this->last_pitch, 80);
                r = {0xFF, 0xFF, 0xFF};
            } else {
                r = {0x80, 0x80, 0x80};
            }
        } else {
            r = {0x40, 0x40, 0x40};
        }            
    }
    return r;       
}
