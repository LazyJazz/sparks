#pragma once

#include <cmath>    // *** for pointing out the change in new g++ compilers ***
#include <cstdlib>  // *** Thanks to Leonhard Gruenschloss and Mike Giles   ***
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "grassland/grassland.h"

namespace sparks {
std::vector<uint32_t> SobolTableGen(unsigned N,
                                    unsigned D,
                                    const std::string &dir_file);
}
