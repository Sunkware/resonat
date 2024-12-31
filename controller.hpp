#ifndef _CONTROLLER_HPP
#define _CONTROLLER_HPP

#include "ensemble.hpp"
#include "echoes.hpp"

struct Controller {
	Ensemble* ensemble;
	Echoes* echoes;
	int sync_stage = -2;
	bool do_synth_out = true;
	bool do_echoes_out = false;

	Controller(Ensemble* ensemble, Echoes* echoes) : ensemble(ensemble), echoes(echoes) {}
};

#endif