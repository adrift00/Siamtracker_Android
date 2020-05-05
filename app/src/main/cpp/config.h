#pragma once

#include <vector>
#include <string>

typedef struct Config {
    std::string MODEL_TYPE = "mobi";
    std::vector<float> SCALES{8};
    std::vector<float> RATIOS{0.33f, 0.5f, 1, 2, 3};
    int STRIDE = 8;
    int ANCHOR_NUM = 5;
    int INSTANCE_SIZE = 255;
    int EXAMPLAR_SIZE = 127;
    int BASE_SIZE = 8;
    float PENALTY_K = 0.04f;
    float WINDOW_INFLUENCE = 0.40f;
    float TRACK_LR = 0.50f;
    int OUT_CHANNELS = 256;
    std::string EXAMPLAR_MODEL_NAME = "siamrpn_mobi_examplar.mnn";
    std::string SEARCH_MODEL_NAME = "siamrpn_mobi_search.mnn";

    //TODO: add the constructor is too ugly, find other way to use the designated initializer.
    Config(
            std::string _MODEL_TYPE,
            std::vector<float> _SCALES, std::vector<float> _RATIOS, int _STRIDE, int _ANCHOR_NUM,
            int _INSTANCE_SIZE, int _EXAMPLAR_SIZE, int _BASE_SIZE,
            float _PENALTY_K, float _WINDOW_INFLUENCE, float _TRACK_LR,
            int _OUT_CHANNELS, std::string _EXAMPLAR_MODEL_NAME, std::string _SEARCH_MODEL_NAME
    ) : MODEL_TYPE(_MODEL_TYPE),
        SCALES(_SCALES), RATIOS(_RATIOS), STRIDE(_STRIDE), ANCHOR_NUM(_ANCHOR_NUM),
        INSTANCE_SIZE(_INSTANCE_SIZE), EXAMPLAR_SIZE(_EXAMPLAR_SIZE), BASE_SIZE(_BASE_SIZE),
        PENALTY_K(_PENALTY_K), WINDOW_INFLUENCE(_WINDOW_INFLUENCE), TRACK_LR(_TRACK_LR),
        OUT_CHANNELS(_OUT_CHANNELS), EXAMPLAR_MODEL_NAME(_EXAMPLAR_MODEL_NAME),
        SEARCH_MODEL_NAME(_SEARCH_MODEL_NAME) {};

} Config;


extern Config cfg;

extern Config alex_cfg;

extern Config mobi_cfg;
extern Config mobi_pruning_cfg;




