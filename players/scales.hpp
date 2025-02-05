#ifndef _SCALES_HPP
#define _SCALES_HPP

#include <vector>

using namespace std;

namespace scales {

const auto JAPENTA_61_2 = vector<int>{61, 62, 64, 65, 66,   73, 74, 76, 77, 78}; // Japanese pentatonic
const auto MAJOR_60_2 = vector<int>{60, 62, 64, 65, 67, 69, 71, 72,   74, 76, 77, 79, 81, 83, 84};
const auto MINOR_60_2 = vector<int>{60, 62, 63, 65, 67, 68, 70, 72,   74, 75, 77, 79, 80, 82, 84};

}

#endif