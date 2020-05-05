#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <MNN/Interpreter.hpp>
#include <opencv2/opencv.hpp>
#include "anchor.h"
#include "config.h"
class SiamTracker{
public:
    SiamTracker(std::string model_path, std::string model_type);
    void init(cv::Mat& img, Rect bbox);
    Rect track(cv::Mat& img);

private:
    std::shared_ptr<AnchorGenerator> anchor_generator_; //use ptr instead of object, because can't initialize object in .h.
    int score_size_;
    std::vector<BBox> all_anchors_;
    cv::Scalar channel_average_;
    std::vector<float> bbox_pos_;
    std::vector<float> bbox_size_;
    std::vector<float> window_;
    //examplar session
    std::shared_ptr<MNN::Interpreter> examplar_interp_;
    MNN::Session* examplar_sess_;
    MNN::Tensor* examplar_input_;
    //search session
    std::shared_ptr<MNN::Interpreter> search_interp_;
    MNN::Session* search_sess_;
    std::vector<MNN::Tensor*> search_input_;
    //util functions
    float size_z(std::vector<float>& bbox_size);
    float size_x(std::vector<float>& bbox_size);
    cv::Mat get_subwindows(cv::Mat& img, std::vector<float>& pos, int dst_size, int ori_size, cv::Scalar padding);
    //windows
    std::vector<float> hanning(int n);
    std::vector<float> outer(std::vector<float>& vec1, std::vector<float>& vec2);
    // tensor host for examplar output
    std::vector<MNN::Tensor*> examplar_output_hosts_;
};





