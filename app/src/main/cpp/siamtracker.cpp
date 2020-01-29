#include <jni.h>
#include <android/log.h>
#include <string>
#include "siamrpn.h"

SiamRPN_MNN *tracker;
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
extern "C" JNIEXPORT void JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerInitModel(
        JNIEnv *env,
        jobject obj,
        jstring path
) {
    const char *modelPath = env->GetStringUTFChars(path, 0);
    tracker = new SiamRPN_MNN(modelPath);

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerInit(
        JNIEnv *env,
        jobject obj,
        jstring img_path_,
        jfloatArray bbox_
) {
    const char *img_path = env->GetStringUTFChars(img_path_, 0);
    cv::Mat init_img = cv::imread(img_path);
    Rect init_bbox;
    jboolean iscopy = true;
    float *bbox = env->GetFloatArrayElements(bbox_, &iscopy);
    init_bbox.cx = bbox[0];
    init_bbox.cy = bbox[1];
    init_bbox.w = bbox[2];
    init_bbox.h = bbox[3];
    tracker->init(init_img, init_bbox);
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerTrack(
        JNIEnv *env,
        jobject obj,
        jstring img_path_
) {
    const char *img_path = env->GetStringUTFChars(img_path_, 0);
    cv::Mat search_img = cv::imread(img_path);
    Rect bbox = tracker->track(search_img);
    std::cout << bbox.cx << " " << bbox.cy << " " << bbox.w << " " << bbox.h << std::endl;
}
