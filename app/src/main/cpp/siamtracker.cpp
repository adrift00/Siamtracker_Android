#include <jni.h>
#include <android/log.h>
#include <string>
#include <opencv2/imgproc/types_c.h>
#include <chrono>
#include "siamrpn.h"

SiamRPN_MNN *tracker;
#define TAG "cpp log"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
extern "C" JNIEXPORT void JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerInitModel(
        JNIEnv *env,
        jobject obj,
        jstring path,
        jstring model_type_
) {
    const char *modelPath = env->GetStringUTFChars(path, 0);
    const char *modelType=env->GetStringUTFChars(model_type_,0);
    tracker = new SiamRPN_MNN(modelPath,modelType);

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerInit(
        JNIEnv *env,
        jobject obj,
        jbyteArray yuv_bytes,
        jint width,
        jint height,
        jfloatArray bbox_
) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    unsigned char *yuv_ptr = (unsigned char *) (env->GetByteArrayElements(yuv_bytes, 0));
    cv::Mat yuv_img(int(height * 1.5), width, CV_8UC1);
    ::memcpy(yuv_img.data, yuv_ptr, (size_t) (width * height * 1.5));
    cv::Mat init_img(height, width, CV_8UC3);
    cv::cvtColor(yuv_img, init_img, CV_YUV2BGR_NV12);
    Rect init_bbox;
    float *bbox = env->GetFloatArrayElements(bbox_, 0);
    init_bbox.cx = bbox[0];
    init_bbox.cy = bbox[1];
    init_bbox.w = bbox[2];
    init_bbox.h = bbox[3];
    //debug
    tracker->init(init_img, init_bbox);
    cv::Rect rect(int(bbox[0] - bbox[2] / 2.f), int(bbox[1] - bbox[3] / 2), int(bbox[2]), int(bbox[3]));
    cv::rectangle(init_img, rect, cv::Scalar(0, 0, 255), 1, cv::LINE_8, 0);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - begin);
    float span_time = static_cast<float>(time_span.count());
    LOGI("cpp init time: %f", span_time);
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerTrack(
        JNIEnv *env,
        jobject obj,
        jbyteArray yuv_bytes,
        jint width,
        jint height
) {

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    unsigned char *yuv_ptr = (unsigned char *) (env->GetByteArrayElements(yuv_bytes, 0));
    cv::Mat yuv_img(int(height * 1.5), width, CV_8UC1);
    ::memcpy(yuv_img.data, yuv_ptr, (size_t) (width * height * 1.5));
    cv::Mat search_img(height, width, CV_8UC3);
    cv::cvtColor(yuv_img, search_img, CV_YUV2BGR_NV12);
//    cv::imwrite("/storage/emulated/0/search.jpg", search_img);
//    search_img=cv::imread("/storage/emulated/0/siamtracker/00000006.jpg");
    Rect bbox = tracker->track(search_img);
    float bbox_ptr[4] = {bbox.cx, bbox.cy, bbox.w, bbox.h};
    jfloatArray rect = env->NewFloatArray(4);
    env->SetFloatArrayRegion(rect, 0, 4, bbox_ptr);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - begin);
    float span_time = static_cast<float>(time_span.count());
    LOGI("cpp track time: %f", span_time);
    return rect;
}
