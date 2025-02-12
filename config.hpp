#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <memory>

namespace cfg {

// Primary

const size_t CHANNELS = 2;
const size_t SAMPLERATE = 0x8000;
const size_t BLOCKSIZE = 0x200;
const double DURATION = 10.0; // sec
const double DELAY = 1.0; // sec, from "reading head" to "writing head"
const double WEIGHT = 0.0625;
const int FRAMERATE = 16;
const size_t WIDTH = 1300; // > 0x100, the width of momentary spectrum
const double AVERFADE_WEIGHT = 0.9;

// Derived

const size_t BLOCKMEMSIZE = BLOCKSIZE * CHANNELS * sizeof(int16_t);
const size_t BANDWIDTH = BLOCKSIZE >> 1;
const size_t BLOCKS = size_t(DURATION * SAMPLERATE / BLOCKSIZE);

}

#endif