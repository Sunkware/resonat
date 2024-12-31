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

#include "drummer.hpp"
#include "gmtimbres.hpp"
#include "../soundfonts.hpp"

using namespace std;

Drummer::Drummer(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel) {
    this->chan_tt = new_channel++;
    this->chan_d = new_channel++;
    fluid_synth_program_select(synth, this->chan_tt, sfids[SFIDS::TINY], 0, GMSS::WOODBLOCK);
    fluid_synth_program_select(synth, this->chan_d, sfids[SFIDS::LARGE], 0x80, 0); // drums
    this->last_tt_pitch = 60;
}

tuple<uint8_t, uint8_t, uint8_t> Drummer::react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats) {
    tuple<uint8_t, uint8_t, uint8_t> r = {0, 0, 0};
    // Tick-tock
    if ((i_blk & 0xF) == 4) {
        fluid_synth_noteoff(synth, this->chan_tt, this->last_tt_pitch);
        this->last_tt_pitch = 128 - this->last_tt_pitch;
        fluid_synth_noteon(synth, this->chan_tt, this->last_tt_pitch, 70);
        fluid_synth_cc(synth, this->chan_tt, 10, 64 + (this->last_tt_pitch - 64) * 15); // panorama
        r = {0, 0xFF, 0};
    }
    // Drum
    if ((i_blk & 0xF) == 8) {
        if (spectrum_stats.mean > 0x80) {
            fluid_synth_noteoff(synth, this->chan_d, GMPM::ACOUSTIC_SNARE);
            fluid_synth_noteon(synth, this->chan_d, GMPM::ACOUSTIC_SNARE, 80);
            r = {0, get<1>(r), 0xFF};
        } else {
            r = {0, get<1>(r), 0x40};
        }            
    }
    return r;
}
