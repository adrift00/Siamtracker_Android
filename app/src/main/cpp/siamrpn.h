#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <MNN/Interpreter.hpp>
#include <opencv2/opencv.hpp>
#include "anchor.h"
#include "config.h"

class SiamRPN_MNN {
public:
    SiamRPN_MNN(std::string model_dir);

    void init(cv::Mat img, Rect bbox);

    Rect track(cv::Mat img);

private:
    AnchorGenerator anchor_generator_;
    int score_size_;
    std::vector<BBox> all_anchors_;
    cv::Scalar channel_average_;
    std::vector<float> bbox_pos_;
    std::vector<float> bbox_size_;
    //examplar session
    MNN::Interpreter *exam_interp_;
    MNN::Session *exam_sess_;
    MNN::Tensor *exam_input_;
    //search session
    MNN::Interpreter *search_interp_;
    MNN::Session *search_sess_;
    std::vector<MNN::Tensor *> search_input_;


    std::vector<float> window_;

    int size_z(std::vector<float> bbox_size);

    int size_x(std::vector<float> bbox_size);

    cv::Mat get_subwindows(cv::Mat img, std::vector<float> pos, int dst_size, int ori_size,
                           cv::Scalar padding);


};





