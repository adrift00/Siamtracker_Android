#include "config.h"

Config cfg{
        "mobi",//std::string MODEL_TYPE
        {8}, //std::vector<float> SCALES
        {0.33f, 0.5f, 1, 2, 3}, //std::vector<float> RATIOS
        8, //int STRIDE
        5, //int ANCHOR_NUM
        255, //int INSTANCE_SIZE
        127, //int EXAMPLAR_SIZE
        8, //int BASE_SIZE
        0.04f, //float PENALTY_K
        0.4f, //float WINDOW_INFLUENCE
        0.5f, //float TRACK_LR
        256, //int OUT_CHANNELS
        "siamrpn_mobi_examplar.mnn", //std::string EXAMPLAR_MODEL_NAME
        "siamrpn_mobi_search.mnn" //std::string SEARCH_MODEL_NAME
};
Config alex_cfg{
        "alex",//std::string MODEL_TYPE
        {8}, //std::vector<float> SCALES
        {0.33f, 0.5f, 1, 2, 3}, //std::vector<float> RATIOS
        8, //int STRIDE
        5, //int ANCHOR_NUM
        255, //int INSTANCE_SIZE
        127, //int EXAMPLAR_SIZE
        0, //int BASE_SIZE
        0.16f, //float PENALTY_K
        0.4f, //float WINDOW_INFLUENCE
        0.3f, //float TRACK_LR
        256, //int OUT_CHANNELS
        "siamrpn_alex_examplar.mnn", //std::string EXAMPLAR_MODEL_NAME
        "siamrpn_alex_search.mnn" //std::string SEARCH_MODEL_NAME
};

Config mobi_cfg{
        "mobi",//std::string MODEL_TYPE
        {8}, //std::vector<float> SCALES
        {0.33f, 0.5f, 1, 2, 3}, //std::vector<float> RATIOS
        8, //int STRIDE
        5, //int ANCHOR_NUM
        255, //int INSTANCE_SIZE
        127, //int EXAMPLAR_SIZE
        8, //int BASE_SIZE
        0.04f, //float PENALTY_K
        0.4f, //float WINDOW_INFLUENCE
        0.5f, //float TRACK_LR
        256, //int OUT_CHANNELS
        "siamrpn_mobi_examplar.mnn", //std::string EXAMPLAR_MODEL_NAME
        "siamrpn_mobi_search.mnn" //std::string SEARCH_MODEL_NAME

};

Config mobi_pruning_cfg{
        "mobi_pruning",//std::string MODEL_TYPE
        {8}, //std::vector<float> SCALES
        {0.33f, 0.5f, 1, 2, 3}, //std::vector<float> RATIOS
        8, //int STRIDE
        5, //int ANCHOR_NUM
        255, //int INSTANCE_SIZE
        127, //int EXAMPLAR_SIZE
        8, //int BASE_SIZE
        0.04f, //float PENALTY_K
        0.4f, //float WINDOW_INFLUENCE
        0.5f, //float TRACK_LR
        192, //int OUT_CHANNELS
        "siamrpn_mobi_pruning_examplar.mnn", //std::string EXAMPLAR_MODEL_NAME
        "siamrpn_mobi_pruning_search.mnn" //std::string SEARCH_MODEL_NAME
};

