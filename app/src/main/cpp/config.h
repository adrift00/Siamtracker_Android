#pragma once

#include <vector>

typedef struct Config {
    std::vector<float> SCALES{8};
    std::vector<float> RATIOS{0.33, 0.5, 1, 2, 3};
    int STRIDE = 8;
    int INSTANCE_SIZE = 287;
    int EXAMPLAR_SIZE = 127;
    int BASE_SIZE = 0;
    float PENALTY_K = 0.16;
    float WINDOW_INFLUENCE = 0.40;
    float TRACK_LR = 0.30;
} Config;


extern Config cfg;



