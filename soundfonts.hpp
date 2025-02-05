#ifndef _SOUNDFONTS_HPP
#define _SOUNDFONTS_HPP

#include <memory>

using namespace std;

const char* const SOUNDFONTS_DIRPATH_ENVAR_NAME = "SOUNDFONTS_DIRPATH";
const auto SOUNDFONTS_FILENAMES = {
	"arachno10.sf2",
	"jnsgm2.sf2",
	"mt32.sf2"
};

// Use whatever naming scheme is appropriate for you,
// and they must be in the same order as corresponding SOUNDFONTS_FILENAMES
enum SFIDS {
    LARGE = 0,
    SMALL = 1,
    TINY = 2
};

#endif