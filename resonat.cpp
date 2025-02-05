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

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <stdio.h>

#include "config.hpp"
#include "controller.hpp"
#include "echoes.hpp"
#include "ensemble.hpp"
#include "streams.hpp"

using namespace std;

const auto VERSION = "2025.02.05";

const auto ON_SYMB = "✓";
const auto OFF_SYMB = "✗";

int64_t time_musec() {
	return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int main() {
	printf("ReSonat v%s © Sunkware\n", VERSION);

	printf("Starting: ensemble… ");
	fflush(stdout);

	Ensemble ensemble;

	auto n_players = ensemble.get_players_num();

	printf("synth, %lu soundfonts, %lu players ✅ echoes… ", ensemble.get_sfids_num(), n_players);
	fflush(stdout);

	Echoes echoes;

	if (echoes.load() == 0) {
		printf("loaded ");
	} else {
		printf("inited ");
	}

	auto ctrl = Controller{&ensemble, &echoes};

	printf("%lu blocks ✅ streams… ", cfg::BLOCKS);
	fflush(stdout);

	Streams streams;

	streams.start(&ctrl);

	printf("✅ tables… ");
	fflush(stdout);

	auto offscaletab = vector<vector<size_t>>(cfg::BANDWIDTH);
	for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
		offscaletab[y] = vector<size_t>(cfg::WIDTH - 0x100);
		for (size_t x = 0; x < (cfg::WIDTH - 0x100); x++) {
			offscaletab[y][x] = (size_t(double(x) * cfg::BLOCKS / (cfg::WIDTH - 0x100)) * cfg::BANDWIDTH + (cfg::BANDWIDTH - 1 - y)) * cfg::CHANNELS;
		}
	}

	vector<size_t> widthmodtab(cfg::WIDTH << 1);
	for (size_t i = 0; i < (cfg::WIDTH << 1); i++) {
		widthmodtab[i] = i % cfg::WIDTH;
	}

	printf("✅\nKeys (at ReSonat window, not here):\nQ - quit, E - toggle echoes output, S - toggle synth output, R - toggle render\n");
	fflush(stdout);

	auto framebuf = cv::Mat(2 + cfg::BLOCKSIZE + n_players, cfg::WIDTH, CV_8UC4);
	uint32_t* fbdata_ptr;

	bool quit = false;

	bool do_render = true;

	auto t_imag_start = time_musec() - echoes.runtime;
	echoes.runtime = 0;

	while (!quit) {

		if (do_render) {
			
			size_t src_offs;

			// Echoes spectrogram
			fbdata_ptr = (uint32_t*)framebuf.data;
			auto echoes_sg_data = echoes.spectrogram.data();
			for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
				for (size_t x = 0; x < (cfg::WIDTH - 0x100); x++) {
					src_offs = offscaletab[y][x];
					*fbdata_ptr = (((uint32_t)echoes_sg_data[src_offs]) << 8) + (((uint32_t)echoes_sg_data[src_offs + 1]) << 0x10); // green & red
					fbdata_ptr++;
				}
				fbdata_ptr += 0x100;
			}
			int pos_blk = echoes.pos_blk_read * (cfg::WIDTH - 0x100) / cfg::BLOCKS;
			cv::line(framebuf, cv::Point{pos_blk, 0}, cv::Point{pos_blk, cfg::BANDWIDTH - 1}, cv::Scalar{0xFF, 0, 0}); // playing head
			pos_blk = echoes.pos_blk_write * (cfg::WIDTH - 0x100) / cfg::BLOCKS;
			cv::line(framebuf, cv::Point{pos_blk, 0}, cv::Point{pos_blk, cfg::BANDWIDTH - 1}, cv::Scalar{0, 0, 0}); // recording head
			// Echoes fading-average momentary spectrum
			auto fbdata_row_ptr = ((uint32_t*)framebuf.data) + cfg::WIDTH - 0x100;
			auto spg = ensemble.sliding_averfade_spectrum.data() +  cfg::BANDWIDTH - 1;
			for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
				auto avener = (uint32_t)(*spg);
				auto avener_color = 0x80 + (avener >> 1);
				fbdata_ptr = fbdata_row_ptr;
				for (int x = 0; x < avener; x++) {
					*fbdata_ptr = avener_color; // blue
					fbdata_ptr++;
				}
				memset(fbdata_ptr, 0, (0x100 - avener) << 2); // rest of the line is black
				spg--;
				fbdata_row_ptr += cfg::WIDTH;
			}

			cv::line(framebuf, cv::Point{0, cfg::BANDWIDTH}, cv::Point{cfg::WIDTH - 1, cfg::BANDWIDTH}, cv::Scalar{0x80, 0, 0});

			// Cannot use ensemble.pos_blk itself, because it can be updated by another thread in out_callback(),
			// in the middle of the following 2 drawings
			auto ensemble_pos_blk = ensemble.pos_blk;

			// Eventogram
			for (size_t x = 0; x < cfg::WIDTH; x++) {
				auto ex = widthmodtab[x + ensemble_pos_blk];
				fbdata_ptr = ((uint32_t*)framebuf.data) + (1 + cfg::BANDWIDTH) * cfg::WIDTH + x;
				auto evg = ensemble.eventogram.data() + (ex * n_players * 3);
				for (size_t y = 0; y < n_players; y++) {
					*fbdata_ptr = ((uint32_t)(*evg)) + (((uint32_t)(*(evg + 1))) << 8) + (((uint32_t)(*(evg + 2))) << 0x10);
					fbdata_ptr += cfg::WIDTH;
					evg += 3;
				}
			}

			cv::line(framebuf, cv::Point{0, (int)(1 + cfg::BANDWIDTH + n_players)}, cv::Point{cfg::WIDTH - 1, (int)(1 + cfg::BANDWIDTH + n_players)}, cv::Scalar{0, 0x80, 0});

			// Synth spectrogram
			for (size_t x = 0x100; x < cfg::WIDTH; x++) {
				auto ex = widthmodtab[x + ensemble_pos_blk];
				fbdata_ptr = ((uint32_t*)framebuf.data) + (2 + cfg::BANDWIDTH + n_players) * cfg::WIDTH + x - 0x100;
				auto spg = ensemble.spectrogram.data() + ((ex * cfg::BANDWIDTH + cfg::BANDWIDTH - 1 ) * cfg::CHANNELS);
				for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
					*fbdata_ptr = ((uint32_t)(*spg)) + (((uint32_t)(*(spg + 1))) << 0x10); // blue & red
					spg -= cfg::CHANNELS;
					fbdata_ptr += cfg::WIDTH;
				}
			}
			// Synth momentary spectrum
			fbdata_row_ptr = ((uint32_t*)framebuf.data) + ((2 + cfg::BANDWIDTH + n_players) * cfg::WIDTH) + cfg::WIDTH - 0x100;
			spg = ensemble.spectrogram.data() + ((widthmodtab[ensemble_pos_blk + cfg::WIDTH - 1] * cfg::BANDWIDTH + cfg::BANDWIDTH - 1 ) * cfg::CHANNELS);
			for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
				auto avener = (((uint32_t)(*spg)) + ((uint32_t)(*(spg + 1)))) >> 1;
				auto avener_color = (0x80 + (avener >> 1)) << 8; // green
				fbdata_ptr = fbdata_row_ptr;
				for (int x = 0; x < avener; x++) {
					*fbdata_ptr = avener_color;
					fbdata_ptr++;
				}
				memset(fbdata_ptr, 0, (0x100 - avener) << 2); // rest of the line is black
				spg -= cfg::CHANNELS;
				fbdata_row_ptr += cfg::WIDTH;
			}

			cv::imshow("ReSonat", framebuf);
		}

		int key = cv::waitKey(int(1000 / cfg::FRAMERATE));
		switch (key) {
			case 'q':
			case 'Q':
				quit = true;
				break;
			case 'e':
			case 'E':
				ctrl.do_echoes_out = !ctrl.do_echoes_out;
				break;
			case 's':
			case 'S':
				ctrl.do_synth_out = !ctrl.do_synth_out;
				break;
			case 'r':
			case 'R':
				do_render = !do_render;
				if (!do_render) {
					// Clear window
					memset(framebuf.data, 0x40, ((2 + cfg::BLOCKSIZE + n_players) * cfg::WIDTH) << 2);
					cv::imshow("ReSonat", framebuf);
				}
				break;
		}

		echoes.runtime = time_musec() - t_imag_start;
		double runtime_sec = 1e-6 * echoes.runtime;

		auto echoes_toggle_symb = ctrl.do_echoes_out ? ON_SYMB : OFF_SYMB;
		auto synth_toggle_symb = ctrl.do_synth_out ? ON_SYMB : OFF_SYMB;
		auto render_toggle_symb = do_render ? ON_SYMB : OFF_SYMB;
		printf("\rRuntime %.3f sec | %5.1f %% of lap %d | Echoes out %s | Synth out %s | Render %s | {W-R}=%lu       ", runtime_sec, 100.0 * echoes.pos_blk_read / cfg::BLOCKS, int(runtime_sec / cfg::DURATION), echoes_toggle_symb, synth_toggle_symb, render_toggle_symb, (cfg::BLOCKS + echoes.pos_blk_write - echoes.pos_blk_read) % cfg::BLOCKS);
		fflush(stdout);
	}

	cv::destroyAllWindows();

	printf("\nStopping: streams… ");
	fflush(stdout);

	streams.stop();

	printf("✅\nSaving: echoes… ");
	fflush(stdout);

	echoes.save();

	printf("✅\n");

	return 0;
}