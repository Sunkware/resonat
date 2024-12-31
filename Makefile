CXXFLAGS := -std=c++11 -O2

resonat: resonat.cpp config.hpp controller.hpp echoes.hpp ensemble.hpp streams.hpp echoes.o ensemble.o streams.o players/drummer.o players/flutist.o players/pianist.o players/singer.o
	rm -f $@
	c++ $(CXXFLAGS) $< echoes.o ensemble.o streams.o players/drummer.o players/flutist.o players/pianist.o players/singer.o -lfluidsynth -lopencv_core -lopencv_highgui -lopencv_imgproc -lportaudio -o $@

streams.o: streams.cpp streams.hpp config.hpp controller.hpp echoes.hpp ensemble.hpp
	rm -f $@
	c++ $(CXXFLAGS) $< -c -o $@

echoes.o: echoes.cpp echoes.hpp config.hpp
	rm -f $@
	c++ $(CXXFLAGS) $< -c -o $@

ensemble.o: ensemble.cpp ensemble.hpp config.hpp soundfonts.hpp spectrumstats.hpp players/*.hpp
	rm -f $@
	c++ $(CXXFLAGS) $< -c -o $@

players/%.o: players/%.cpp players/%.hpp players/player.hpp players/gmtimbres.hpp config.hpp soundfonts.hpp spectrumstats.hpp
	rm -f $@
	c++ $(CXXFLAGS) $< -c -o $@

clean:
	rm -f players/*.o
	rm -f *.o
	rm -f resonat

run:
	./resonat

pack:
	rm -f resonat*.7z
	7z a -mx9 -t7z '-x!resonat' '-xr!*.o' '-x!_run_' '-x!.vscode' resonat_$(shell date +%Y.%m.%d)_0.7z ./.