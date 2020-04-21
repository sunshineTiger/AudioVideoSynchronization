//
// Created by zh on 2020/4/13.
//

#include "include/JavaCallHelper.h"



JavaCallHelper::JavaCallHelper(JavaVM *_javaVm, JNIEnv *_env, jobject &_obj):javaVM(_javaVm),env(_env) {
    //设置成全局的obj
    this->obj = env->NewGlobalRef(_obj);
    jclass jClazz = env->GetObjectClass(obj);
    //对应java层的三个方法
    jmid_prepare = env->GetMethodID(jClazz, "onPrepare", "()V");
    jmid_progress = env->GetMethodID(jClazz, "onProgress", "(I)V");
    jmid_error = env->GetMethodID(jClazz, "onError", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {}

void JavaCallHelper::onError(int thread, int errorCode) {
    sendMessage(thread, errorCode, jmid_error);
}

void JavaCallHelper::onProgress(int thread, int progress) {
    sendMessage(thread, progress, jmid_progress);
}

void JavaCallHelper::onPrepare(int thread) {
    sendMessage(thread, NULL, jmid_prepare);
}


void JavaCallHelper::sendMessage(int thread, int params, jmethodID methodID) {

    if (thread == THREAD_CHILD) {
        JNIEnv *bindEnv = NULL;
        //绑定到主线程
        if (javaVM->AttachCurrentThread(&bindEnv, 0) != JNI_OK) {
            return;
        }
//        if (!bindEnv && !obj) {
            if (params == NULL)
                bindEnv->CallVoidMethod(obj, methodID);
            else
                bindEnv->CallVoidMethod(obj, methodID, params);
//        }
        //解绑
        javaVM->DetachCurrentThread();
    } else {
        if (!env && !obj)
            env->CallVoidMethod(obj, methodID, params);
    }
}
