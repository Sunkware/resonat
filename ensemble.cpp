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

#include <fluidsynth.h>
#include <opencv2/core.hpp>

#include "ensemble.hpp"
#include "players/drummer.hpp"
#include "players/flutist.hpp"
#include "players/pianist.hpp"
#include "players/singer.hpp"
#include "soundfonts.hpp"
#include "spectrumstats.hpp"

template<class P>
void Ensemble::add_player() {
#ifdef _cpp_lib_make_unique // compiler supports C++14 or later
	this->players.push_back(make_unique<P>(this->synth, this->sfids, this->new_channel));
#else // compiler supports only C++11
	this->players.push_back(move(unique_ptr<P>(new P(this->synth, this->sfids, this->new_channel))));
#endif
}

Ensemble::Ensemble() {
	this->fls_settings = new_fluid_settings();
	fluid_settings_setnum(this->fls_settings, "synth.sample-rate", cfg::SAMPLERATE);
	this->synth = new_fluid_synth(this->fls_settings);

	this->new_channel = 0;

	this->block1d = vector<double>(cfg::BLOCKSIZE);
	this->spectrum = vector<double>(cfg::BLOCKSIZE);

	this->pos_blk = 0;

	this->sliding_averfade_spectrum = vector<uint8_t>(cfg::BANDWIDTH);
	this->spectrogram = vector<uint8_t>(cfg::WIDTH * cfg::BANDWIDTH * cfg::CHANNELS);

	auto soundfonts_dirpath = getenv(SOUNDFONTS_DIRPATH_ENVAR_NAME);
	if (soundfonts_dirpath == NULL) {
		fprintf(stderr, "\"%s\" environment variable is not set, assuming empty.\n", SOUNDFONTS_DIRPATH_ENVAR_NAME);
	}
	auto soundfonts_dirpath_str = (soundfonts_dirpath == NULL) ? string() : string(soundfonts_dirpath);
	for (auto fname : SOUNDFONTS_FILENAMES) {
		auto fpath = soundfonts_dirpath_str + "/" + fname;
		this->sfids.push_back(fluid_synth_sfload(this->synth, fpath.c_str(), 0));
	}

	this->add_player<Drummer>();
	this->add_player<Flutist>();
	this->add_player<Pianist>();
	this->add_player<Singer>();

	this->eventogram = vector<uint8_t>(cfg::WIDTH * this->players.size() * 3);
}

void Ensemble::react_and_read(vector<uint8_t>& spectrogram, size_t i_blk, int16_t* output) {
	SpectrumStats spectrum_stats{0, -1.0, 0.0};

	auto spc = this->sliding_averfade_spectrum.data();
	auto spg = spectrogram.data() + i_blk * (cfg::BANDWIDTH * cfg::CHANNELS);
	for (size_t i = 0; i < cfg::BANDWIDTH; i++) {	
		*spc = uint8_t(cfg::AVERFADE_WEIGHT * (*spc) + (1.0 - cfg::AVERFADE_WEIGHT) * 0.5 * ((*spg) + (*(spg + 1))));
		
		spectrum_stats.mean += *spc;
		if (*spc > spectrum_stats.max) {
			spectrum_stats.max = *spc;
			spectrum_stats.argmax = i;
		}
		
		spc++;
		spg += cfg::CHANNELS;
	}
	spectrum_stats.mean /= cfg::BANDWIDTH;
	
	auto evg = this->eventogram.data() + (this->pos_blk * this->players.size() * 3);
	for (size_t i = 0; i < this->players.size(); i++) {
		auto r = this->players[i]->react(this->synth, spectrogram, i_blk, spectrum_stats);
		*evg = get<0>(r);
		evg++;
		*evg = get<1>(r);
		evg++;
		*evg = get<2>(r);
		evg++;
	}

	fluid_synth_write_s16(this->synth, cfg::BLOCKSIZE, output, 0, cfg::CHANNELS, output, 1, cfg::CHANNELS);

	// Update slice of synth spectrogram
	double scale = 1.0 / 32768.0;
	double re, im, lum;
	for (size_t c = 0; c < cfg::CHANNELS; c++) {
		auto src = output + c;
		auto blk = this->block1d.data();
		for (size_t i = 0; i < cfg::BLOCKSIZE; i++) {
			*blk = double(*src) * scale;
			blk++;
			src += cfg::CHANNELS;
		}
		cv::dft(this->block1d, this->spectrum);
		auto spc = this->spectrum.data() + 1; // skip "freq 0"
		auto spg = this->spectrogram.data() + (this->pos_blk * cfg::BANDWIDTH * cfg::CHANNELS) + c;
		for (size_t i = 0; i < cfg::BANDWIDTH; i++) {
			re = *spc;
			im = ((i + 1) < cfg::BANDWIDTH) ? *(spc + 1) : 0.0;
			lum = (8 + log10(1e-8 + re * re + im * im)) / 12;
			if (lum > 1.0) {
				lum = 1.0;
			}
			*spg = uint8_t(0xFF * lum);
			spc += 2;
			spg += cfg::CHANNELS;
		}
	}

	this->pos_blk = (this->pos_blk + 1) % cfg::WIDTH;
}

size_t Ensemble::get_sfids_num() {
	return this->sfids.size();
}

size_t Ensemble::get_players_num() {
	return this->players.size();
}

Ensemble::~Ensemble() {
	delete_fluid_synth(this->synth);
	delete_fluid_settings(this->fls_settings);
}
