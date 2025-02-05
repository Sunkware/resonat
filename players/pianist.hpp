#ifndef _PIANIST_HPP
#define _PIANIST_HPP

#include "player.hpp"
#include "scales.hpp"

class Pianist : public Player {

    int chan;
    int last_pitch1;
    int last_pitch2;
    int last_pitch3;
    const vector<int>& scale = scales::MINOR_60_2;

public:

    Pianist(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel);
    tuple<uint8_t, uint8_t, uint8_t> react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats);

};

#endif