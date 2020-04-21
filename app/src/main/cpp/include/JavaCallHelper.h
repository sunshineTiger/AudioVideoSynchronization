//
// Created by zh on 2020/4/13.
//

#ifndef AUDIOVIDEOSYNCHRONIZATION_JAVACALLHELPER_H
#define AUDIOVIDEOSYNCHRONIZATION_JAVACALLHELPER_H

#include <jni.h>
#include "comm.h"

const int THREAD_CHILD = 1;
const int FFMPEG_CAN_NOT_OPEN_URL = 10;
const int FFMPEG_CAN_NOT_FIND_STREAMS = 12;
const int FFMPEG_FIND_DECODER_FAIL = 13;
const int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 14;
const int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 15;
const int FFMPEG_OPEN_DECODER_FAIL = 16;
const int FFMPEG_NOMEDIA = 17;

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *javaVm, JNIEnv *env, jobject &obj);

    ~JavaCallHelper();

    void onError(int thread, int errorCode);

    void onProgress(int thread, int errorCode);

    void onPrepare(int thread);

private:
    void sendMessage(int thread, int errorCode, jmethodID methodID);

    JavaVM *javaVM;
    JNIEnv *env;
    jobject obj = 0;
    jmethodID jmid_prepare;
    jmethodID jmid_progress;
    jmethodID jmid_error;
};


#endif //AUDIOVIDEOSYNCHRONIZATION_JAVACALLHELPER_H
