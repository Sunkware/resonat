#ifndef _ENSEMBLE_HPP
#define _ENSEMBLE_HPP

#include <fluidsynth.h>

#include <vector>

#include "players/player.hpp"

using namespace std;

class Ensemble {

	fluid_settings_t* fls_settings;
	fluid_synth_t* synth;
	int new_channel;
	vector<int> sfids;
	vector<unique_ptr<Player>> players;
	vector<double> block1d; // to avoid allocations in callback
	vector<double> spectrum; // to avoid allocations in callback

	template<class P>
	void add_player();

public:
	
	size_t pos_blk;
	vector<uint8_t> sliding_averfade_spectrum;
	vector<uint8_t> spectrogram;
	vector<uint8_t> eventogram;

	Ensemble();

	void react_and_read(vector<uint8_t>& spectrogram, size_t i_blk, int16_t* output);
	size_t get_sfids_num();
	size_t get_players_num();
	
	~Ensemble();

};

#endif