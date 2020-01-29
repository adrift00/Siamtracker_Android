#include "anchor.h"

#pragma optimize("", off)

std::vector<BBox> AnchorGenerator::generate_base_anchor() {
    std::vector<BBox> base_anchor(anchor_num_);
    int size = stride_ * stride_;
    int count = 0;
    for (float r : ratios_) {
        int ws = int(sqrtf(size * 1. / r));
        int hs = int(ws * r);
        for (float s : scales_) {
            float w = ws * s;
            float h = hs * s;
            base_anchor[count].x1 = -w * 0.5;
            base_anchor[count].y1 = -h * 0.5;
            base_anchor[count].x2 = w * 0.5;
            base_anchor[count].y2 = h * 0.5;
            count++;
        }
    }
    return base_anchor;
}

#pragma optimize("", on)

#pragma optimize("", off)

std::vector<BBox> AnchorGenerator::generate_all_anchors(int img_c, int out_size) {
    int begin = img_c - int(out_size / 2) * stride_;
    int all_anchor_num = out_size * out_size * anchor_num_;
    std::vector<BBox> all_anchors(all_anchor_num);
    std::vector<BBox> base_anchors = generate_base_anchor();
    for (int i = 0; i < base_anchors.size(); i++) {
        base_anchors[i].x1 += begin;
        base_anchors[i].x2 += begin;
        base_anchors[i].y1 += begin;
        base_anchors[i].y2 += begin;
    }
    //for (int x = 0; x < out_size; x++) {
    //    for (int y = 0; y < out_size; y++) {
    //        float shift_x = x * stride_;
    //        float shift_y = y * stride_;
    //        for (int i = 0; i < anchor_num_; i++) {
    //            int idx = x * (out_size * anchor_num_) + y * anchor_num_ + i;
    //            all_anchors[idx].x1 = shift_x + base_anchors[i].x1;
    //            all_anchors[idx].y1 = shift_y + base_anchors[i].y1;
    //            all_anchors[idx].x2 = shift_x + base_anchors[i].x2;
    //            all_anchors[idx].y2 = shift_y + base_anchors[i].y2;
    //        }
    //    }
    //}
    for (int i = 0; i < anchor_num_; i++) {
        for (int y = 0; y < out_size; y++) {
            for (int x = 0; x < out_size; x++) {
                float shift_x = x * stride_;
                float shift_y = y * stride_;
                int idx = i * (out_size * out_size) + y * out_size + x;
                all_anchors[idx].x1 = shift_x + base_anchors[i].x1;
                all_anchors[idx].y1 = shift_y + base_anchors[i].y1;
                all_anchors[idx].x2 = shift_x + base_anchors[i].x2;
                all_anchors[idx].y2 = shift_y + base_anchors[i].y2;
            }
        }
    }
    return all_anchors;
}

#pragma optimize("", on)

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}



