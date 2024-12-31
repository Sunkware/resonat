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

const char* VERSION = "2024.12.31_1";

int64_t time_musec() {
	return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int main() {
	printf("ReSonat v%s (c) Sunkware \n", VERSION);

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
		offscaletab[y] = vector<size_t>(cfg::WIDTH);
		for (size_t x = 0; x < cfg::WIDTH; x++) {
			offscaletab[y][x] = (size_t(double(x) * cfg::BLOCKS / cfg::WIDTH) * cfg::BANDWIDTH + (cfg::BANDWIDTH - 1 - y)) * cfg::CHANNELS;
		}
	}

	vector<size_t> widthmodtab(cfg::WIDTH << 1);
	for (size_t i = 0; i < (cfg::WIDTH << 1); i++) {
		widthmodtab[i] = i % cfg::WIDTH;
	}

	printf("✅\nKeys (at ReSonat window, not here):\nQ - quit, E - toggle echoes output, S - toggle synth output, R - toggle render\n");
	fflush(stdout);

	auto framebuf = cv::Mat(2 + cfg::BLOCKSIZE + n_players, cfg::WIDTH, CV_8UC4);

	bool quit = false;

	bool do_render = true;

	auto t_imag_start = time_musec() - echoes.runtime;
	echoes.runtime = 0;

	while (!quit) {

		if (do_render) {
			
			size_t src_offs;
			size_t dst_offs = 0;

			// Echoes spectrogram
			auto echoes_sg_data = echoes.spectrogram.data();
			for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
				for (size_t x = 0; x < cfg::WIDTH; x++) {
					src_offs = offscaletab[y][x];
					framebuf.data[dst_offs] = 0;
					framebuf.data[dst_offs + 1] = echoes_sg_data[src_offs]; // green
					framebuf.data[dst_offs + 2] = echoes_sg_data[src_offs + 1]; // red
					dst_offs += 4;
				}
			}
			int pos_blk = echoes.pos_blk_read * cfg::WIDTH / cfg::BLOCKS;
			cv::line(framebuf, cv::Point{pos_blk, 0}, cv::Point{pos_blk, cfg::BANDWIDTH - 1}, cv::Scalar{0xFF, 0, 0});
			pos_blk = echoes.pos_blk_write * cfg::WIDTH / cfg::BLOCKS;
			cv::line(framebuf, cv::Point{pos_blk, 0}, cv::Point{pos_blk, cfg::BANDWIDTH - 1}, cv::Scalar{0, 0, 0});

			cv::line(framebuf, cv::Point{0, cfg::BANDWIDTH}, cv::Point{cfg::WIDTH - 1, cfg::BANDWIDTH}, cv::Scalar{0x80, 0, 0});

			// Cannot use ensemble.pos_blk itself, because it can be updated by another thread in out_callback,
			// in the middle of the following 2 drawings
			auto ensemble_pos_blk = ensemble.pos_blk;

			// Eventogram
			for (size_t x = 0; x < cfg::WIDTH; x++) {
				auto ex = widthmodtab[x + ensemble_pos_blk];
				dst_offs = ((1 + cfg::BANDWIDTH) * cfg::WIDTH + x) << 2;
				auto evg = ensemble.eventogram.data() + (ex * n_players * 3);
				for (size_t y = 0; y < n_players; y++) {
					framebuf.data[dst_offs] = *evg;
					evg++;
					framebuf.data[dst_offs + 1] = *evg;
					evg++;
					framebuf.data[dst_offs + 2] = *evg;
					evg++;
					dst_offs += cfg::WIDTH << 2;
				}
			}

			cv::line(framebuf, cv::Point{0, (int)(1 + cfg::BANDWIDTH + n_players)}, cv::Point{cfg::WIDTH - 1, (int)(1 + cfg::BANDWIDTH + n_players)}, cv::Scalar{0, 0x80, 0});

			// Synth spectrogram
			for (size_t x = 0; x < cfg::WIDTH; x++) {
				auto ex = widthmodtab[x + ensemble_pos_blk];
				dst_offs = ((2 + cfg::BANDWIDTH + n_players) * cfg::WIDTH + x) << 2;
				auto spg = ensemble.spectrogram.data() + ((ex * cfg::BANDWIDTH + cfg::BANDWIDTH - 1 ) * cfg::CHANNELS);
				for (size_t y = 0; y < cfg::BANDWIDTH; y++) {
					framebuf.data[dst_offs] = *spg; // blue
					framebuf.data[dst_offs + 2] = *(spg + 1); // red
					spg -= cfg::CHANNELS;
					dst_offs += cfg::WIDTH << 2;
				}
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
				break;
		}

		echoes.runtime = time_musec() - t_imag_start;
		double runtime_sec = 1e-6 * echoes.runtime;

		auto echoes_toggle_symb = ctrl.do_echoes_out ? "✓" : "✗";
		auto synth_toggle_symb = ctrl.do_synth_out ? "✓" : "✗";
		auto render_toggle_symb = do_render ? "✓" : "✗";
		printf("\rRuntime %.3f sec | %5.1f %% of lap %d | Echoes out %s | Synth out %s | Render %s | {W-R}=%lu       ", runtime_sec, 100.0 * echoes.pos_blk_read / cfg::BLOCKS, int(runtime_sec / cfg::DURATION), echoes_toggle_symb, synth_toggle_symb, render_toggle_symb, (cfg::BLOCKS + echoes.pos_blk_write - echoes.pos_blk_read) % cfg::BLOCKS);
		fflush(stdout);
	}

	cv::destroyAllWindows();

	printf("\nStopping: streams… ");
	fflush(stdout);

	streams.stop();

	printf("✅\nSaving… ");
	fflush(stdout);

	echoes.save();

	printf("✅\n");

	return 0;
}