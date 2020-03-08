#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <cmath>

struct BBox {
    float x1;
    float y1;
    float x2;
    float y2;
};

struct Rect {
    float cx;
    float cy;
    float w;
    float h;
};

class AnchorGenerator {
private:
    std::vector<float> scales_;
    std::vector<float> ratios_;
    int stride_;
    int base_size_;
    int anchor_num_;

    std::vector<BBox> generate_base_anchor();


public:
    AnchorGenerator(std::vector<float> scales, std::vector<float> ratios, int stride) :
            scales_(scales),
            ratios_(ratios),
            stride_(stride),
            base_size_(stride),
            anchor_num_(scales.size()*ratios.size())
    {}
    std::vector<BBox> generate_all_anchors(int img_c, int out_size);
};

float clip(float n, float lower, float upper);
