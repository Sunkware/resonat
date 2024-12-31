#ifndef _ECHOES_HPP
#define _ECHOES_HPP

#include <memory>
#include <vector>

using namespace std;

class Echoes {

	vector<int16_t> data;
	vector<double> block1d; // to avoid allocations in callback
	vector<double> spectrum; // to avoid allocations in callback

public:

	size_t pos_blk_read;
	size_t pos_blk_write;
	int64_t runtime; // microseconds
	vector<uint8_t> spectrogram;

	Echoes();

	void read_add(int16_t* output, bool silence);
	void write(int16_t* input);
	void sync_pos_blk_write();
	void save();
	int load();

};

#endif