#include <jni.h>
#include <android/log.h>
#include <string>
#include <opencv2/imgproc/types_c.h>
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
        jbyteArray yuv_bytes,
        jint width,
        jint height,
        jfloatArray bbox_
) {
    unsigned char *yuv_ptr = (unsigned char *) (env->GetByteArrayElements(yuv_bytes, 0));
    cv::Mat yuv_img(int(height * 1.5), width, CV_8UC1);
    ::memcpy(yuv_img.data, yuv_ptr, (size_t) (width * height * 1.5));
    cv::Mat init_img(height, width, CV_8UC3);
    cv::cvtColor(yuv_img, init_img, CV_YUV2BGR_NV12);
    cv::imwrite("/storage/emulated/0/examplar.jpg", init_img);
    Rect init_bbox;
    float *bbox = env->GetFloatArrayElements(bbox_, 0);
    init_bbox.cx = bbox[0];
    init_bbox.cy = bbox[1];
    init_bbox.w = bbox[2];
    init_bbox.h = bbox[3];
    cv::Rect rect(bbox[0]-bbox[2]/2,bbox[1]-bbox[3]/2,bbox[2],bbox[3]);
    cv::rectangle(init_img, rect, cv::Scalar(0, 0, 255),1, cv::LINE_8,0);
    cv::imwrite("/storage/emulated/0/examplar_line.jpg",init_img);
    tracker->init(init_img, init_bbox);
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_siamtracker_MainActivity_siamtrackerTrack(
        JNIEnv *env,
        jobject obj,
        jbyteArray yuv_bytes,
        jint width,
        jint height
) {
    unsigned char *yuv_ptr = (unsigned char *) (env->GetByteArrayElements(yuv_bytes, 0));
    cv::Mat yuv_img(int(height * 1.5), width, CV_8UC1);
    ::memcpy(yuv_img.data, yuv_ptr, (size_t) (width * height * 1.5));
    cv::Mat search_img(height, width, CV_8UC3);
    cv::cvtColor(yuv_img, search_img, CV_YUV2BGR_NV12);
    cv::imwrite("/storage/emulated/0/search.jpg", search_img);
    Rect bbox = tracker->track(search_img);
    float bbox_ptr[4] = {bbox.cx, bbox.cy, bbox.w, bbox.h};
    jfloatArray rect = env->NewFloatArray(4);
    env->SetFloatArrayRegion(rect, 0, 4, bbox_ptr);
    return rect;
}
