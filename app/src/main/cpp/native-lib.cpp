#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <FFmpegHelper.h>
#include "JavaCallHelper.h"

ANativeWindow *native_window;
FFmpegHelper *fFmpegHelper;
JavaCallHelper *javaCallHelper;
JavaVM *javaVm = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

void renderFrame(uint8_t *data, int lineSize, int w, int h) {
    ANativeWindow_setBuffersGeometry(native_window, w, h, WINDOW_FORMAT_RGBA_8888);
    // 定义绘图缓冲区
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(native_window, &window_buffer, 0)) {
        ANativeWindow_release(native_window);
        native_window = 0;
        return;
    }

//    缓冲区  window_data[0] =255;
    uint8_t *window_data = static_cast<uint8_t *>(window_buffer.bits);
//    r     g   b  a int 一行的像素 字节拷贝 需要x4
    int window_linesize = window_buffer.stride * 4;
    //数据源
    uint8_t *src_data = data;

    //一个字节拷贝速度太慢
    //一行一行的拷贝
    //不能整体拷贝原因：缓冲区一行的大小可能会比实际的一行数据大，整体拷贝会导致是在缓存去一行会去填充实际的两行数据，导致视频画面乱码
    for (int i = 0; i < window_buffer.height; ++i) {
        //大小以左边为准，根据真实为准
        memcpy(window_data + i * window_linesize, src_data + i * lineSize, window_linesize);
    }
    ANativeWindow_unlockAndPost(native_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_audiovideosynchronization_FFmpegSurface_native_1set_1surface(JNIEnv *env,
                                                                              jobject thiz,
                                                                              jobject surface) {
    if (native_window) {
        ANativeWindow_release(native_window);
        native_window = 0;
    }
    native_window = ANativeWindow_fromSurface(env, surface);
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_audiovideosynchronization_FFmpegSurface_native_1start(JNIEnv *env, jobject thiz) {
    if (fFmpegHelper) {
        fFmpegHelper->setRenderFrameCallBack(renderFrame);
        fFmpegHelper->start();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_audiovideosynchronization_FFmpegSurface_native_1prepare(JNIEnv *env,
                                                                         jobject instance,
                                                                         jstring path) {
    const char *str_ = env->GetStringUTFChars(path, NULL);
    javaCallHelper = new JavaCallHelper(javaVm, env, instance);
    fFmpegHelper = new FFmpegHelper(javaCallHelper, str_);
    fFmpegHelper->prepare();
    env->ReleaseStringUTFChars(path, str_);
}