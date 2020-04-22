#pragma once
#include <vector>
#include <string>

typedef struct Config {
    std::vector<float> SCALES{ 8 };
    std::vector<float> RATIOS{ 0.33f,0.5f,1,2,3 };
    int STRIDE = 8;
    int ANCHOR_NUM = 5;
    int INSTANCE_SIZE = 255;
    int EXAMPLAR_SIZE = 127;
    int BASE_SIZE = 8;
    float PENALTY_K = 0.04f;
    float WINDOW_INFLUENCE = 0.40f;
    float TRACK_LR = 0.50f;
    int OUT_CHANNELS=256;
    std::string EXAMPLAR_MODEL_NAME="siamrpn_mobi_examplar.mnn";
    std::string SEARCH_MODEL_NAME="siamrpn_mobi_search.mnn";


}Config;



extern Config cfg;

extern Config alex_cfg;

extern Config mobi_cfg;
extern Config mobi_pruning_cfg;




