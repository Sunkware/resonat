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

#include <cstring>
#include <stdio.h>

#include "config.hpp"
#include "streams.hpp"

int in_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	auto ctrl = (Controller*)userData;
	if (ctrl->sync_stage == -1) {
		ctrl->echoes->sync_pos_blk_write();
		ctrl->sync_stage = 0;
	}
	if (ctrl->sync_stage == 0) {
		ctrl->echoes->write((int16_t*)input); // updates slice of echoes spectrogram
	}
	if (statusFlags & paInputOverflow) {
		fprintf(stderr, "InputOverflow\n");
	}
	if (statusFlags & paInputUnderflow) {
		fprintf(stderr, "InputUnderflow\n");
	}
	return paContinue;
}

int out_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	auto ctrl = (Controller *)userData;
	ctrl->ensemble->react_and_read(ctrl->echoes->spectrogram, ctrl->echoes->pos_blk_read, (int16_t*)output); // updates slice of synth spectrogram
	if (!ctrl->do_synth_out) {
		memset(output, 0, cfg::BLOCKMEMSIZE);
	}	
	ctrl->echoes->read_add((int16_t*)output, !ctrl->do_echoes_out);
	if (ctrl->sync_stage == -2) {
		ctrl->sync_stage = -1;
	}
	if (statusFlags & paOutputOverflow) {
		fprintf(stderr, "OutputOverflow\n");
	}
	if (statusFlags & paOutputUnderflow) {
		fprintf(stderr, "OutputUnderflow\n");
	}
	if (statusFlags & paPrimingOutput) {
		fprintf(stderr, "PrimingOutput\n");
	}
	return paContinue;
}

void Streams::start(Controller* ctrl) {
	// Suppress ALSA lib warnings (see https://github.com/PortAudio/portaudio/issues/463)
	// From https://stackoverflow.com/questions/24778998/how-to-disable-or-re-route-alsa-lib-logging
	// and then https://stackoverflow.com/questions/40576003/ignoring-warning-wunused-result
	(void)!freopen("/dev/null", "w", stderr);
	Pa_Initialize();
	(void)!freopen("/dev/tty", "w", stderr);
	// FIXME: non-ALSA errors may pass undetected this way... and what about cross-platformness?

    // Maybe Pa_OpenDefaultStream() doesn't care, maybe it does...
    this->in_stream = NULL;
    this->out_stream = NULL;

	Pa_OpenDefaultStream(
		&(this->in_stream),
		cfg::CHANNELS, 
		0, // input only
		paInt16,
		cfg::SAMPLERATE,
		cfg::BLOCKSIZE,
		in_callback,
		ctrl
	);

	Pa_OpenDefaultStream(
		&(this->out_stream),
		0, // output only
		cfg::CHANNELS,
		paInt16,
		cfg::SAMPLERATE,
		cfg::BLOCKSIZE,
		out_callback,
		ctrl
	);

	Pa_StartStream(in_stream);
	Pa_StartStream(out_stream);
}

void Streams::stop() {
    Pa_StopStream(this->in_stream);
	Pa_StopStream(this->out_stream);

	Pa_CloseStream(this->in_stream);
	Pa_CloseStream(this->out_stream);

	Pa_Terminate();
}