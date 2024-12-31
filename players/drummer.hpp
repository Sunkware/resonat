#ifndef _DRUMMER_HPP
#define _DRUMMER_HPP

#include "player.hpp"

class Drummer : public Player {

    int chan_tt;
    int chan_d;
    int last_tt_pitch;

public:

    Drummer(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel);
    tuple<uint8_t, uint8_t, uint8_t> react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats);

};

#endif