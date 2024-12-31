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

#include <opencv2/core.hpp>

#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

#include "config.hpp"
#include "echoes.hpp"

const char* RUN_DIRNAME = "_run_";
const char* COUNTERS_FILENAME = "counters.bin";
const char* DATA_FILENAME = "data.bin";
const char* SPECTROGRAM_FILENAME = "spectrogram.bin";

Echoes::Echoes() {
	this->pos_blk_read = 0;
	this->pos_blk_write = 0;
	this->runtime = 0;
	this->data = vector<int16_t>(cfg::BLOCKS * cfg::BLOCKSIZE * cfg::CHANNELS);
	this->spectrogram = vector<uint8_t>(cfg::BLOCKS * cfg::BANDWIDTH * cfg::CHANNELS);
	this->block1d = vector<double>(cfg::BLOCKSIZE);
	this->spectrum = vector<double>(cfg::BLOCKSIZE);
}

void Echoes::read_add(int16_t* output, bool silence) {
	if (!silence) {
		auto src = this->data.data() + this->pos_blk_read * cfg::BLOCKSIZE * cfg::CHANNELS;
		for (size_t i = 0; i < cfg::BLOCKSIZE; i++) {
			for (size_t j = 0; j < cfg::CHANNELS; j++) {
				*output += *src;
				src++;
				output++;
			}
		}
	}
	this->pos_blk_read++;
	if (this->pos_blk_read == cfg::BLOCKS) {
		this->pos_blk_read = 0;
	}
}

void Echoes::write(int16_t* input) {
	auto dst_start = this->data.data() + this->pos_blk_write * cfg::BLOCKSIZE * cfg::CHANNELS;

	auto dst = dst_start;
	for (size_t i = 0; i < cfg::BLOCKSIZE; i++) {
		for (size_t c = 0; c < cfg::CHANNELS; c++) {
			(*dst) = int16_t(cfg::WEIGHT * (*input) + cfg::COMPLEMENT_WEIGHT * (*dst));
			dst++;
			input++;
		}			
	}

	// Update slice of spectrogram
	double scale = 1.0 / 32768.0;
	double re, im, lum;
	for (size_t c = 0; c < cfg::CHANNELS; c++) {
		dst = dst_start + c;
		auto blk = this->block1d.data();
		for (size_t i = 0; i < cfg::BLOCKSIZE; i++) {
			*blk = double(*dst) * scale;
			blk++;
			dst += cfg::CHANNELS; 
		}
		cv::dft(this->block1d, this->spectrum);
		auto spc = this->spectrum.data() + 1; // skip "freq 0"
		auto spg = this->spectrogram.data() + (this->pos_blk_write) * (cfg::BANDWIDTH * cfg::CHANNELS) + c;
		for (size_t i = 0; i < cfg::BANDWIDTH; i++) {
			re = (*spc);
			im = ((i + 1) < cfg::BANDWIDTH) ? (*(spc + 1)) : 0.0;
			lum = (8 + log10(1e-8 + re * re + im * im)) / 12;
			if (lum > 1.0) {
				lum = 1.0;
			}
			*spg = uint8_t(0xFF * lum);
			spc += 2;
			spg += cfg::CHANNELS;
		}
	}

	this->pos_blk_write++;
	if (this->pos_blk_write == cfg::BLOCKS) {
		this->pos_blk_write = 0;
	}
}

void Echoes::sync_pos_blk_write() {
	this->pos_blk_write = (this->pos_blk_read + size_t(cfg::DELAY * cfg::SAMPLERATE) / cfg::BLOCKSIZE) % cfg::BLOCKS;
}

void Echoes::save() {
	mkdir(RUN_DIRNAME, 0777);

	ofstream ofs;
	ofs.open(string(RUN_DIRNAME) + "/" + string(COUNTERS_FILENAME), ios::binary | ios::out);
	ofs.write((char*)&(this->pos_blk_read), sizeof(this->pos_blk_read));
	ofs.write((char*)&(this->runtime), sizeof(this->runtime));
	ofs.close();

	ofs.open(string(RUN_DIRNAME) + "/" + string(DATA_FILENAME), ios::binary | ios::out);
	ofs.write((char *)this->data.data(), cfg::BLOCKS * cfg::BLOCKSIZE * cfg::CHANNELS * sizeof(int16_t));
	ofs.close();

	ofs.open(string(RUN_DIRNAME) + "/" + string(SPECTROGRAM_FILENAME), ios::binary | ios::out);
	ofs.write((char *)this->spectrogram.data(), cfg::BLOCKS * cfg::BANDWIDTH * cfg::CHANNELS);
	ofs.close();
}

int Echoes::load() {
	DIR* drun;
	if ((drun = opendir(RUN_DIRNAME)) == NULL) {
		return -1;
	} else {
		closedir(drun);
	}

	ifstream ifs;
	ifs.open(string(RUN_DIRNAME) + "/" + string(COUNTERS_FILENAME), ios::binary | ios::in);
	ifs.read((char*)&(this->pos_blk_read), sizeof(this->pos_blk_read));
	ifs.read((char*)&(this->runtime), sizeof(this->runtime));
	ifs.close();
	this->pos_blk_read = this->pos_blk_read % cfg::BLOCKS; // untrusted input...

	ifs.open(string(RUN_DIRNAME) + "/" + string(DATA_FILENAME), ios::binary | ios::in);
	ifs.read((char *)this->data.data(), cfg::BLOCKS * cfg::BLOCKSIZE * cfg::CHANNELS * sizeof(int16_t));
	ifs.close();

	ifs.open(string(RUN_DIRNAME) + "/" + string(SPECTROGRAM_FILENAME), ios::binary | ios::in);
	ifs.read((char *)this->spectrogram.data(), cfg::BLOCKS * cfg::BANDWIDTH * cfg::CHANNELS);
	ifs.close();

	return 0;
}
