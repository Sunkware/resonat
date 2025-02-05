#ifndef _PLAYER_HPP
#define _PLAYER_HPP

#include <fluidsynth.h>

#include <tuple>
#include <vector>

#include "../config.hpp"
#include "../spectrumstats.hpp"

using namespace std;

class Player {

public:

    virtual tuple<uint8_t, uint8_t, uint8_t> react(fluid_synth_t* synth, const vector<uint8_t>& spectrogram, size_t i_blk, const SpectrumStats& spectrum_stats) = 0;

    virtual ~Player() = default;
};

#endif