#ifndef _SINGER_HPP
#define _SINGER_HPP

#include "player.hpp"
#include "scales.hpp"

class Singer : public Player {

    int chan;
    int last_pitch;
    const vector<int>& scale = scales::MINOR_60_2;

public:

    Singer(fluid_synth_t* synth, const vector<int>& sfids, int& new_channel);
    tuple<uint8_t, uint8_t, uint8_t> react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats);

};

#endif