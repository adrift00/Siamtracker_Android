#include "siam_tracker.h"
#include "include/opencv2/core/hal/interface.h"
#include "include/MNN/Interpreter.hpp"
#include "include/MNN/MNNForwardType.h"
#include <jni.h>

#define PI 3.1415926

#include <android/log.h>

#define TAG "cpp log"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)

SiamTracker::SiamTracker(std::string model_path, std::string model_type) {
    if (model_type == "mobi") {
        cfg = mobi_cfg;
    } else if (model_type == "mobi_pruning") {
        cfg = mobi_pruning_cfg;
    } else if (model_type == "alex") {
        cfg = alex_cfg;
    } else {
        LOGI("model type invalid!");
    }
    anchor_generator_ = std::make_shared<AnchorGenerator>(cfg.SCALES,
                                                          cfg.RATIOS,
                                                          cfg.STRIDE,
                                                          cfg.BASE_SIZE);
    score_size_ = int((cfg.INSTANCE_SIZE - cfg.EXAMPLAR_SIZE) / cfg.STRIDE) + 1 + cfg.BASE_SIZE;
    std::vector<float> h = hanning(score_size_);
    window_ = outer(h, h);
    all_anchors_ = anchor_generator_->generate_all_anchors(int(cfg.INSTANCE_SIZE / 2), score_size_);
    //examplar
    std::string examplar_path = model_path + cfg.EXAMPLAR_MODEL_NAME;
    examplar_interp_ = std::shared_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile(examplar_path.c_str()));
    MNN::ScheduleConfig sched_cfg;
//    the time use precision low has no affect, because my cpu is too old.
//    MNN::BackendConfig backendConfig;
//    backendConfig.precision=MNN::BackendConfig::Precision_Low;
//    sched_cfg.backendConfig=&backendConfig;
    examplar_sess_ = examplar_interp_->createSession(sched_cfg);
    examplar_input_ = examplar_interp_->getSessionInput(examplar_sess_, nullptr);
    //search
    std::string search_path = model_path + cfg.SEARCH_MODEL_NAME;
    search_interp_ = std::shared_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile(search_path.c_str()));
    search_sess_ = search_interp_->createSession(sched_cfg);
    if (model_type == "alex") {
        search_input_.push_back(search_interp_->getSessionInput(search_sess_, "e0"));
        // the MNN will add the input, if not add this, the result fo these branch will be wrong.
        search_input_.push_back(search_interp_->getSessionInput(search_sess_, "71"));
    } else {
        search_input_.push_back(search_interp_->getSessionInput(search_sess_, "e0"));
        search_input_.push_back(search_interp_->getSessionInput(search_sess_, "e1"));
        search_input_.push_back(search_interp_->getSessionInput(search_sess_, "e2"));
        if (model_type == "mobi_pruning") {
            search_input_.push_back(search_interp_->getSessionInput(search_sess_, "246"));
            search_input_.push_back(search_interp_->getSessionInput(search_sess_, "303"));
            search_input_.push_back(search_interp_->getSessionInput(search_sess_, "361"));

        }
    }
    search_input_.push_back(search_interp_->getSessionInput(search_sess_, "search"));
    // examplar host tensor
    if (model_type == "alex") {
        examplar_output_hosts_.resize(1); //the alexnet backbone only contains one outputs
    } else {
        examplar_output_hosts_.resize(3);
    }
}

void SiamTracker::init(cv::Mat &img, Rect bbox) {
    std::vector<float> bbox_pos{bbox.cx, bbox.cy};
    std::vector<float> bbox_size{bbox.w, bbox.h};
    float sz = size_z(bbox_size);
    channel_average_ = cv::mean(img);
    cv::Mat examplar = get_subwindows(img, bbox_pos, cfg.EXAMPLAR_SIZE, round(sz), channel_average_);
    examplar.convertTo(examplar, CV_32FC3);
//    cv::imwrite("/storage/emulated/0/examplar_patch.png",examplar);
    //wrap for input tensor
    std::shared_ptr<MNN::Tensor> examplar_host(
            MNN::Tensor::create<float>({1, cfg.EXAMPLAR_SIZE, cfg.EXAMPLAR_SIZE, 3},
                                       nullptr,
                                       MNN::Tensor::TENSORFLOW));
    float *examplar_data = examplar_host->host<float>();
    std::memcpy(examplar_data, examplar.data, examplar_host->size());
    examplar_input_->copyFromHostTensor(examplar_host.get());
    examplar_interp_->runSession(examplar_sess_);
    for (int i = 0; i < examplar_output_hosts_.size(); i++) {
        std::string out_name = "e" + std::to_string(i);
        MNN::Tensor *examplar_output = examplar_interp_->getSessionOutput(examplar_sess_, out_name.c_str());
        std::vector<int> output_size;
        if (cfg.MODEL_TYPE == "alex") {
            output_size = {1, cfg.OUT_CHANNELS, 6, 6};
        } else {
            output_size = {1, cfg.OUT_CHANNELS, 7, 7};
        }
        examplar_output_hosts_[i] = MNN::Tensor::create<float>(output_size,
                                                               nullptr,
                                                               MNN::Tensor::CAFFE);
        examplar_output->copyToHostTensor(examplar_output_hosts_[i]);
        search_input_[i]->copyFromHostTensor(examplar_output_hosts_[i]);
    }
    // the workaround for the mnn added input
    if (cfg.MODEL_TYPE == "alex") {
        search_input_[1]->copyFromHostTensor(examplar_output_hosts_[0]);
    }
    if (cfg.MODEL_TYPE == "mobi_pruning") {
        search_input_[3]->copyFromHostTensor(examplar_output_hosts_[0]);
        search_input_[4]->copyFromHostTensor(examplar_output_hosts_[1]);
        search_input_[5]->copyFromHostTensor(examplar_output_hosts_[2]);
    }
    bbox_pos_ = bbox_pos;
    bbox_size_ = bbox_size;
}

Rect SiamTracker::track(cv::Mat &img) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    float sz = size_z(bbox_size_);
    float scale_z = cfg.EXAMPLAR_SIZE / float(sz);
    float sx = size_x(bbox_size_);
    cv::Mat search = get_subwindows(img, bbox_pos_, cfg.INSTANCE_SIZE, round(sx), channel_average_);
    search.convertTo(search, CV_32FC3);
//    cv::imwrite("/storage/emulated/0/search_patch.png",search);

    std::shared_ptr<MNN::Tensor> search_host(
            MNN::Tensor::create<float>({1, cfg.INSTANCE_SIZE, cfg.INSTANCE_SIZE, 3}, nullptr,
                                       MNN::Tensor::TENSORFLOW));
    float *search_data = search_host->host<float>();
    std::memcpy(search_data, search.data, search_host->size());

    //NOTE: the e0-2 need to be copied because the MNN will change the input when running, will cost some time.
    for (int i = 0; i < examplar_output_hosts_.size(); i++) {
        search_input_[i]->copyFromHostTensor(examplar_output_hosts_[i]);
    }
    // the workaround for the mnn added input
    if (cfg.MODEL_TYPE == "alex") {
        search_input_[1]->copyFromHostTensor(examplar_output_hosts_[0]);
    }
    if (cfg.MODEL_TYPE == "mobi_pruning") {
        search_input_[3]->copyFromHostTensor(examplar_output_hosts_[0]);
        search_input_[4]->copyFromHostTensor(examplar_output_hosts_[1]);
        search_input_[5]->copyFromHostTensor(examplar_output_hosts_[2]);
    }
    search_input_[search_input_.size() - 1]->copyFromHostTensor(search_host.get());
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - begin);
    float span_time = static_cast<float>(time_span.count());
    LOGI("cpp track convert time: %f", span_time);

    begin = std::chrono::steady_clock::now();

    search_interp_->runSession(search_sess_);

    now = std::chrono::steady_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - begin);
    span_time = static_cast<float>(time_span.count());
    LOGI("cpp track run session time: %f", span_time);

    MNN::Tensor *cls = search_interp_->getSessionOutput(search_sess_, "cls");
    MNN::Tensor *loc = search_interp_->getSessionOutput(search_sess_, "loc");

    std::shared_ptr<MNN::Tensor> cls_host = std::make_shared<MNN::Tensor>(cls, MNN::Tensor::CAFFE);
    std::shared_ptr<MNN::Tensor> loc_host = std::make_shared<MNN::Tensor>(loc, MNN::Tensor::CAFFE);
    cls->copyToHostTensor(cls_host.get());
    loc->copyToHostTensor(loc_host.get());

    float *cls_data = cls_host->host<float>();
    float *loc_data = loc_host->host<float>();
//    debug
//    MNN::Tensor *save_tensor = search_interp_->getSessionOutput(search_sess_, "189");
//
//    std::shared_ptr<MNN::Tensor> save_host = std::make_shared<MNN::Tensor>(save_tensor, MNN::Tensor::CAFFE);
//    save_tensor->copyToHostTensor(save_host.get());
//
//    float *save_data = save_host->host<float>();
//    MNN::Tensor *save_tensor = search_interp_->getSessionOutput(search_sess_, "71");
//
//    std::shared_ptr<MNN::Tensor> save_host = std::make_shared<MNN::Tensor>(cls, MNN::Tensor::CAFFE);
//    save_tensor->copyToHostTensor(save_host.get());
//
//    float *save_data = save_host->host<float>();
//    debug
    int pred_size = score_size_ * score_size_ * cfg.ANCHOR_NUM;
    std::vector<float> score(pred_size);
    std::vector<float> pscore(pred_size);
    std::vector<Rect> pred_bbox(pred_size);
    std::vector<float> penalty(pred_size);

    for (int i = 0; i < pred_size; i++) {
        score[i] = exp(cls_data[pred_size + i]) / (exp(cls_data[i]) + exp(cls_data[pred_size + i]));
        float src_width = all_anchors_[i].x2 - all_anchors_[i].x1;
        float src_height = all_anchors_[i].y2 - all_anchors_[i].y1;
        float src_ctr_x = all_anchors_[i].x1 + 0.5f * src_width;
        float src_ctr_y = all_anchors_[i].y1 + 0.5f * src_height;

        pred_bbox[i].cx = loc_data[i] * src_width + src_ctr_x;
        pred_bbox[i].cy = loc_data[pred_size + i] * src_height + src_ctr_y;
        pred_bbox[i].w = src_width * exp(loc_data[2 * pred_size + i]);
        pred_bbox[i].h = src_height * exp(loc_data[3 * pred_size + i]);
        // penalty
        float r = (bbox_size_[0] / bbox_size_[1]) / (pred_bbox[i].w / pred_bbox[i].h);
        float rc = fmax(r, 1.f / r);
        std::vector<float> s1 = std::vector<float>{bbox_size_[0] * scale_z, bbox_size_[1] * scale_z};
        std::vector<float> s2 = std::vector<float>{pred_bbox[i].w, pred_bbox[i].h};
        float s = size_z(s1) / size_z(s2);
        float sc = fmax(s, 1.f / s);
        penalty[i] = exp(-(rc * sc - 1) * cfg.PENALTY_K);
        pscore[i] = penalty[i] * score[i];
        int idx = i % (score_size_ * score_size_);
        pscore[i] = pscore[i] * (1 - cfg.WINDOW_INFLUENCE) + window_[idx] * cfg.WINDOW_INFLUENCE;
    }
    int best_idx = int(std::max_element(pscore.begin(), pscore.end()) - pscore.begin());
    float best_score = pscore[best_idx];
    Rect best_bbox = pred_bbox[best_idx];

    best_bbox.cx -= cfg.INSTANCE_SIZE / 2;
    best_bbox.cy -= cfg.INSTANCE_SIZE / 2;
    best_bbox.cx /= scale_z;
    best_bbox.cy /= scale_z;
    best_bbox.w /= scale_z;
    best_bbox.h /= scale_z;
    float cx = best_bbox.cx + bbox_pos_[0];
    float cy = best_bbox.cy + bbox_pos_[1];
    float lr = penalty[best_idx] * score[best_idx] * cfg.TRACK_LR;
    float w = bbox_size_[0] * (1 - lr) + lr * best_bbox.w;
    float h = bbox_size_[1] * (1 - lr) + lr * best_bbox.h;
    //clip
    best_bbox.cx = clip(cx, 0, img.cols);
    best_bbox.cy = clip(cy, 0, img.rows);
    best_bbox.w = clip(w, 10, img.cols);
    best_bbox.h = clip(h, 10, img.rows);
    //update
    bbox_pos_[0] = best_bbox.cx;
    bbox_pos_[1] = best_bbox.cy;
    bbox_size_[0] = best_bbox.w;
    bbox_size_[1] = best_bbox.h;
    return best_bbox;
}

float SiamTracker::size_z(std::vector<float> &bbox_size) {
    float context_amount = 0.5;
    float wz = bbox_size[0] + context_amount * (bbox_size[0] + bbox_size[1]);
    float hz = bbox_size[1] + context_amount * (bbox_size[0] + bbox_size[1]);
    float size_z = sqrtf(wz * hz);
    return size_z;
}

float SiamTracker::size_x(std::vector<float> &bbox_size) {
    float context_amount = 0.5;
    float wz = bbox_size[0] + context_amount * (bbox_size[0] + bbox_size[1]);
    float hz = bbox_size[1] + context_amount * (bbox_size[0] + bbox_size[1]);
    float size_z = sqrtf(wz * hz);
    float scale_z = cfg.EXAMPLAR_SIZE / size_z;
    float d_search = (cfg.INSTANCE_SIZE - cfg.EXAMPLAR_SIZE) / 2;
    float pad = d_search / scale_z;
    float size_x = size_z + 2 * pad;
    return size_x;
}

cv::Mat
SiamTracker::get_subwindows(cv::Mat &img, std::vector<float> &pos, int dst_size, int ori_size, cv::Scalar padding) {
    //TODO: the speed of the func maybe slow because of warpAffine
    float scale = float(dst_size) / float(ori_size);
    float shift_x = -floor(pos[0] - (ori_size + 1) / 2.f + 0.5);
    float shift_y = -floor(pos[1] - (ori_size + 1) / 2.f + 0.5);
    cv::Mat mapping = cv::Mat::zeros(2, 3, CV_32FC1);
    mapping.at<float>(0, 0) = 1.0;
    mapping.at<float>(1, 1) = 1.0;
    mapping.at<float>(0, 2) = shift_x;
    mapping.at<float>(1, 2) = shift_y;
    cv::Mat patch;
    cv::warpAffine(img, patch, mapping, cv::Size(ori_size, ori_size), cv::INTER_LINEAR, cv::BORDER_CONSTANT, padding);
    cv::Mat new_patch;
    cv::resize(patch, new_patch, cv::Size(dst_size, dst_size));
    return new_patch;
}

std::vector<float> SiamTracker::hanning(int n) {
    std::vector<float> window(n);
    for (int i = 0; i < n; i++) {
        window[i] = 0.5 - 0.5 * cos(2 * PI * i / (n - 1));
    }
    return window;
}

std::vector<float> SiamTracker::outer(std::vector<float> &vec1, std::vector<float> &vec2) {
    std::vector<float> window(vec1.size() * vec2.size());
    for (int i = 0; i < vec1.size(); i++) {
        for (int j = 0; j < vec2.size(); j++) {
            window[i * vec1.size() + j] = vec1[i] * vec2[j];
        }
    }
    return window;
}









