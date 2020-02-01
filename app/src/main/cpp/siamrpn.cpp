#include "siamrpn.h"

#define PI 3.1415926

std::vector<float> hanning(int n) {
    std::vector<float> window(n);
    for (int i = 0; i < n; i++) {
        window[i] = 0.5 - 0.5 * cos(2 * PI * i / (n - 1));
    }
    return window;
}

std::vector<std::vector<float>> outer(std::vector<float> vec1, std::vector<float> vec2) {
    int n = vec1.size();
    std::vector<std::vector<float>> mat(n, std::vector<float>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            mat[i][j] = vec1[i] * vec2[j];
        }
    }
    return mat;
}

SiamRPN_MNN::SiamRPN_MNN(std::string model_dir) {
    //TODO: INIT OR NOT
    anchor_generator_.init(std::vector<float>{8}, std::vector<float>{0.33, 0.5, 1, 2, 3}, 8);
    score_size_ = int((cfg.INSTANCE_SIZE - cfg.EXAMPLAR_SIZE) / cfg.STRIDE) + 1 + cfg.BASE_SIZE;
    std::vector<float> h = hanning(score_size_);
    auto window = outer(h, h);
    //TODO: 
    for (int i = 0; i < window.size(); i++) {
        for (int j = 0; j < window[0].size(); j++) {
            window_.push_back(window[i][j]);
        }
    }
    all_anchors_ = anchor_generator_.generate_all_anchors(int(cfg.INSTANCE_SIZE / 2), score_size_);
    //examplar
    std::string examplar_path = model_dir + "siamrpn_examplar.mnn";
    exam_interp_ = MNN::Interpreter::createFromFile(examplar_path.c_str());
    MNN::ScheduleConfig conf;
    exam_sess_ = exam_interp_->createSession(conf);
    exam_input_ = exam_interp_->getSessionInput(exam_sess_, nullptr);
    //search
    MNN::ScheduleConfig search_cfg;
    //search_cfg.saveTensors = { "145___tr4146","118___tr4133","132","95___tr4132","95","91","90" };
//    search_cfg.saveTensors = {"92","90"};
    std::string search_path = model_dir + "siamrpn_search.mnn";
    search_interp_ = MNN::Interpreter::createFromFile(search_path.c_str());
    search_sess_ = search_interp_->createSession(search_cfg);
    search_input_.push_back(search_interp_->getSessionInput(search_sess_, "examplar"));
    search_input_.push_back(search_interp_->getSessionInput(search_sess_, "search"));
}

void SiamRPN_MNN::init(cv::Mat img, Rect bbox) {
    std::vector<float> bbox_pos{bbox.cx, bbox.cy};
    std::vector<float> bbox_size{bbox.w, bbox.h};
    float sz = size_z(bbox_size);
    channel_average_ = cv::mean(img);
    cv::Mat examplar_8u = get_subwindows(img, bbox_pos, cfg.EXAMPLAR_SIZE, sz, channel_average_);
    //cv::Mat examplar_8u = cv::imread("E:\\BIP LAB\\siamtracker\\dataset\\examplar.jpg");
    cv::Mat examplar;
    examplar_8u.convertTo(examplar, CV_32F);
    cv::imwrite("/storage/emulated/0/examplar_8u.jpg",examplar_8u);


    //wrap for input tensor
    MNN::Tensor *nhwc_tensor = MNN::Tensor::create<float>(
            std::vector<int>{1, cfg.EXAMPLAR_SIZE, cfg.EXAMPLAR_SIZE, 3}, nullptr,
            MNN::Tensor::TENSORFLOW);
    float *nhwc_data = nhwc_tensor->host<float>();
    size_t nhwc_size = nhwc_tensor->size();
    //TODO: ::
    ::memcpy(nhwc_data, examplar.data, nhwc_size);
    exam_input_->copyFromHostTensor(nhwc_tensor);
    exam_interp_->runSession(exam_sess_);
    MNN::Tensor *exam_output = exam_interp_->getSessionOutput(exam_sess_, nullptr);
    //set the examplar output as search input 
    //the type convert is nessary
    MNN::Tensor *nchw_tensor = MNN::Tensor::create<float>(std::vector<int>{1, 256, 6, 6}, nullptr,
                                                          MNN::Tensor::CAFFE);
    exam_output->copyToHostTensor(nchw_tensor);
    search_input_[0]->copyFromHostTensor(nchw_tensor);
    //exam_output->print();
    bbox_pos_ = bbox_pos;
    bbox_size_ = bbox_size;
}

Rect SiamRPN_MNN::track(cv::Mat img) {
    int sz = size_z(bbox_size_);
    float scale_z = cfg.EXAMPLAR_SIZE / float(sz);
    int sx = size_x(bbox_size_);
    cv::Mat search_8u = get_subwindows(img, bbox_pos_, cfg.INSTANCE_SIZE, sx, channel_average_);
    //debug
    //cv::Mat search_8u = cv::imread("E:\\BIP LAB\\siamtracker\\dataset\\search.png");
    cv::Mat search;

    cv::imwrite("/storage/emulated/0/search_8u.jpg",search_8u);
    search_8u.convertTo(search, CV_32F);
    MNN::Tensor *nhwc_tensor = MNN::Tensor::create<float>(
            std::vector<int>{1, cfg.INSTANCE_SIZE, cfg.INSTANCE_SIZE, 3}, nullptr,
            MNN::Tensor::TENSORFLOW);
    float *nhwc_data = nhwc_tensor->host<float>();
    size_t nhwc_size = nhwc_tensor->size();
    ::memcpy(nhwc_data, search.data, nhwc_size);

    search_input_[1]->copyFromHostTensor(nhwc_tensor);
    search_interp_->runSession(search_sess_);
    MNN::Tensor *cls = search_interp_->getSessionOutput(search_sess_, "cls");
    MNN::Tensor *loc = search_interp_->getSessionOutput(search_sess_, "loc");
    MNN::Tensor *cls_host = new MNN::Tensor(cls, MNN::Tensor::CAFFE);
    MNN::Tensor *loc_host = new MNN::Tensor(loc, MNN::Tensor::CAFFE);
    cls->copyToHostTensor(cls_host);
    loc->copyToHostTensor(loc_host);
    float *cls_ptr = cls_host->host<float>();
    float *loc_ptr = loc_host->host<float>();
    //TODO: ANHCOR_NUM
    int pred_size = score_size_ * score_size_ * 5;
    std::vector<float> score(pred_size);
    std::vector<float> pscore(pred_size);
    std::vector<Rect> pred_bbox(pred_size);
    std::vector<float> penalty(pred_size);
    for (int i = 0; i < pred_size; i++) {
        //score[i] = exp(cls_ptr[2 * i + 1]) / (exp(cls_ptr[2 * i]) + exp(cls_ptr[2 * i + 1]));
        score[i] = exp(cls_ptr[pred_size + i]) / (exp(cls_ptr[i]) + exp(cls_ptr[pred_size + i]));
        float src_width = all_anchors_[i].x2 - all_anchors_[i].x1;
        float src_height = all_anchors_[i].y2 - all_anchors_[i].y1;
        float src_ctr_x = all_anchors_[i].x1 + 0.5 * src_width;
        float src_ctr_y = all_anchors_[i].y1 + 0.5 * src_height;

        pred_bbox[i].cx = loc_ptr[i] * src_width + src_ctr_x;
        pred_bbox[i].cy = loc_ptr[pred_size + i] * src_height + src_ctr_y;
        pred_bbox[i].w = src_width * exp(loc_ptr[2 * pred_size + i]);
        pred_bbox[i].h = src_height * exp(loc_ptr[3 * pred_size + i]);
        // penalty
        float r = (bbox_size_[0] / bbox_size_[1]) / (pred_bbox[i].w / pred_bbox[i].h);
        float rc = fmax(r, 1. / r);
        float s = size_z(std::vector<float>{bbox_size_[0] * scale_z, bbox_size_[1] * scale_z})
                  / size_z(std::vector<float>{pred_bbox[i].w, pred_bbox[i].h});
        float sc = fmax(s, 1. / s);
        penalty[i] = exp(-(rc * sc - 1) * cfg.PENALTY_K);
        pscore[i] = penalty[i] * score[i];
        int idx = i % (score_size_ * score_size_);
        pscore[i] = pscore[i] * (1 - cfg.WINDOW_INFLUENCE) + window_[idx] * cfg.WINDOW_INFLUENCE;
    }
    int best_idx = int(std::max_element(pscore.begin(), pscore.end()) - pscore.begin());
    float best_score = pscore[best_idx];
    Rect best_bbox = pred_bbox[best_idx];
    //
//    int x1 = best_bbox.cx - best_bbox.w / 2.;
//    int y1 = best_bbox.cy - best_bbox.h / 2.;
//    int iw = best_bbox.w;
//    int ih = best_bbox.h;
//    cv::rectangle(search_8u, { x1,y1,iw,ih }, cv::Scalar(0, 0, 255), 1, cv::LINE_8, 0);
//    cv::imshow("search", search_8u);
//    cv::waitKey();


    //
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
    bbox_pos_[0] = cx;
    bbox_pos_[1] = cy;
    bbox_size_[0] = w;
    bbox_size_[1] = h;
    return best_bbox;
}

int SiamRPN_MNN::size_z(std::vector<float> bbox_size) {
    float context_amount = 0.5;
    float wz = bbox_size[0] + context_amount * (bbox_size[0] + bbox_size[1]);
    float hz = bbox_size[1] + context_amount * (bbox_size[0] + bbox_size[1]);
    float size_z = sqrtf(wz * hz);
    return round(size_z);
}

int SiamRPN_MNN::size_x(std::vector<float> bbox_size) {
    float context_amount = 0.5;
    float wz = bbox_size[0] + context_amount * (bbox_size[0] + bbox_size[1]);
    float hz = bbox_size[1] + context_amount * (bbox_size[0] + bbox_size[1]);
    float size_z = sqrtf(wz * hz);
    float scale_z = cfg.EXAMPLAR_SIZE / size_z;
    float d_search = (cfg.INSTANCE_SIZE - cfg.EXAMPLAR_SIZE) / 2;
    float pad = d_search / scale_z;
    float size_x = size_z + 2 * pad;
    return round(size_x);
}

cv::Mat SiamRPN_MNN::get_subwindows(cv::Mat img, std::vector<float> pos, int dst_size, int ori_size,
                                    cv::Scalar padding) {
    float scale = float(dst_size) / float(ori_size);
    float shift_x = -scale * (pos[0] - ori_size / 2);
    float shift_y = -scale * (pos[1] - ori_size / 2);
    cv::Mat mapping = cv::Mat::zeros(2, 3, CV_32FC1);
    mapping.at<float>(0, 0) = scale;
    mapping.at<float>(1, 1) = scale;
    mapping.at<float>(0, 2) = shift_x;
    mapping.at<float>(1, 2) = shift_y;
    cv::Mat patch;
    cv::warpAffine(img, patch, mapping, cv::Size(dst_size, dst_size), cv::INTER_LINEAR,
                   cv::BORDER_CONSTANT, padding);
    //cv::warpAffine(img, patch, mapping, cv::Size(dst_size, dst_size));
    //cv::imshow("patch", patch);
    //cv::waitKey();
    return patch;
}







