#ifndef _STREAMS_HPP
#define _STREAMS_HPP

#include <portaudio.h>

#include "controller.hpp"

class Streams {

    PaStream* in_stream;
    PaStream* out_stream;

public:

    void start(Controller* ctrl);
    void stop();

};

#endif