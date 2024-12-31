# ReSonat

Autonomous software-defined players react in real-time to looped durable long-delay echoes of sound input by playing notes through MIDI software synthesizer to sound output, depending on triggers such as their internal states and energy spectral density of an echoes interval; thus out of feedbacks emerges harmony… or cacophony.

Anoþer obscure producte of [Sunkware](https://sunkware.org).

## Prerequisites

* C++ toolchain with at least C++11 support — check yours [here](https://en.cppreference.com/w/cpp/compiler_support/11), generally any recent one goes. At your option, replace `-std=c++11` by `-std=c++14`, `-std=c++17` etc. in `Makefile`

Libraries:

* [FluidSynth](https://www.fluidsynth.org/)

* [OpenCV](https://opencv.org/)

* [PortAudio](https://www.portaudio.com/)

You need both runtime libraries *and* development files (in particular, headers). For example, on Ubuntu, since packages with the latter have those with the former as dependencies:

```shell
$ sudo apt install libfluidsynth-dev libopencv-dev portaudio19-dev
```

Alternatively, build from sources, e.g. how to build OpenCV on MacOS is described [here](https://docs.opencv.org/4.x/d0/db2/tutorial_macos_install.html) (you do not need everything, please read the entire tutorial first; also, note that there are several ways to build from source… on old versions of MacOS `$ brew install opencv` fails, while plain `CMake`&`make` way works).

* Get soundfonts in `.sf2` format (see links below), put them to some dir, e.g. `~/soundfonts`, and export `SOUNDFONTS_DIRPATH` environment variable pointing to that dir:

```shell
$ export SOUNDFONTS_DIRPATH=$HOME/soundfonts
```

For demo purpose, we use [Arachno](https://www.arachnosoft.com/main/download.php?id=soundfont-sf2) (~140 MB), [Jnsgm2](https://github.com/wrightflyer/SF2_SoundFonts/blob/master/Jnsgm2.sf2) (~32 MB), and [MT32](https://archive.org/download/free-soundfonts-sf2-2019-04/MT32.sf2) (~8 MB) soundfonts, as `arachno10.sf2`, `jnsgm2.sf2`, and `mt32.sf2`. There are lots of others, see for example [this list](https://archive.org/download/free-soundfonts-sf2-2019-04) mentioned in [this Q&A](https://www.reddit.com/r/midi/comments/pmh94q/whats_the_best_allaround_soundfont/), or [obvious search query](https://www.google.com/search?q=download+free+soundfonts).

In `soundfonts.hpp`, you may need to change the `SOUNDFONTS_FILENAMES` constant, which is now

```cpp
const auto SOUNDFONTS_FILENAMES = {
    "arachno10.sf2",
    "jnsgm2.sf2",
    "mt32.sf2"
};
```

and, accordingly, the `SFIDS` enum if you prefer mnemonics other than a soundfont's size.

* Make sure sound input (microphone) and sound output (speakers) work, adjusting levels if needed.

## Quick*start*

```shell
$ cd resonat
$ make
```

The build process usually takes up to 10 seconds. If it fails complaining about missing OpenCV headers, create symlink named `opencv2` in `/usr/local/include/` to `/usr/local/include/opencv4/opencv2/`.

```shell
$ ./resonat
```

or, equivalently,

```shell
$ make run
```

The window appears, consisting of 3 horizontal stripes, from top to bottom:

1. Echoes spectrogram.

2. Ensemble eventogram (few pixels in height, barely noticeable).

3. Synth spectrogram.

In (1), blue line is "playing head" and black line is "recording head". (2) and (3) scroll from right (present) to left (past).

Say something to mic, produce knocking and hissing sounds etc. to "seed" the process.

Listen… … …

Press `Q` to quit. Or other keys to toggle some switches, e.g. `R` pauses rendering and halves CPU usage, low as it is though.

## Windows?

We've assumed Linux (including MacOS flavour) above, although with some modifications it may work in Windows as well, since all 3 libraries are cross-platform.

## Autosave

At exit, the samples, spectrogram, and some counters related to echoes are saved to `_run_` dir; see `Echoes::save()` in `echoes.cpp`. At next start, the playback and rewriting of echoes continues. To start anew, simply delete this dir.

## Motivation

Of course, almost any *existing* music piece can be more or less reproduced by giving player(s) its full score, translated to C++, and appropriate soundfont… this is trivial. Or consider even more degenerate case of one-key instrument with single long "timbre" that contains the whole piece.

But, perhaps, *new* and *interesting* pieces will [emerge](https://en.wikipedia.org/wiki/Emergence) from interaction of seemingly simple players due to loops, [strange attractors](https://en.wikipedia.org/wiki/Attractor#Strange_attractor), (musical) [autocatalyses](https://en.wikipedia.org/wiki/Autocatalysis), and other phenomena inherent to [dynamical systems](https://en.wikipedia.org/wiki/Dynamical_system) with [feedbacks](https://en.wikipedia.org/wiki/Feedback).

Perhaps not.

## Filemap

`drummer.cpp`, `flutist.cpp`, `pianist.cpp`, and `singer.cpp` in `players/` define players' behaviour. This is where either discord or concord stems from. In this demo, most of them base their "decisions" on frequency with the largest energy, i.e. most intensive tone, and on average energy exceeding certain thresholds.

`soundfonts.hpp` lists `.sf2` soundfonts you are going to use. Note that players reference them by values of `SFIDS` enum.

`ensemble.cpp` encapsulates players, soundfonts, and FluidSynth synthesizer.

`echoes.cpp` implements ring buffer of echoes, kind of software-defined tape recorder with short looped tape. `cfg::WEIGHT` parameter in `write()`, being less than 1, simulates that issue of recording head when it does not overwrite previous record completely, — "echoes" we deal with here are *not* of usual reverberation type.

`streams.cpp` handles PortAudio streams and updates echoes and ensemble through callbacks.

`controller.hpp` declares the structure by means of which callbacks interact with echoes and ensemble.

`config.hpp` contains some global parameters such as aforementioned weight, samplerate, and duration of echoes loop.

Finally, `resonat.cpp` with `main()` exploits them all, but also deals with visualisation and user input via OpenCV, whose sophisticated Computer Vision algorithms are completely unused here… for now.

And then there is `Makefile`, which orchestrates building and related tasks (note `$ make pack`).

## Kindred

Here on GitHub alone, already there are *at least*

* [dennisppaul/wellen](https://github.com/dennisppaul/wellen)

* [fabianostermann/EAR-Drummer](https://github.com/fabianostermann/EAR-Drummer)

* [krmnn/MIDIVelocityZone](https://github.com/krmnn/MIDIVelocityZone)

* [theloni-monk/pitch2synth-rs](https://github.com/theloni-monk/pitch2synth-rs)

* [tmhglnd/drumcode](https://github.com/tmhglnd/drumcode)

* [unclechu/MIDI-Trigger](https://github.com/unclechu/MIDI-Trigger)

* [vloop/tap2midi](https://github.com/vloop/tap2midi)

although we haven't looked into whether "output —> input" closure is an integral part of them, due to our laziness and fear of admitting the degree of epigonity.

## Curse of Memory Stairwell

is a pretentious title we suggest for the "art installation" you obtain from this thing when you

* switch off synth output (press `S`),

* switch on echoes output (`E`),

* adjust input and output levels to prevent feedback loop (good external mic helps too),

* and spend several minutes in audio-rich environment (room with talking people, perhaps music) while it is running.