//
// Created by zh on 2020/4/13.
//

#ifndef AUDIOVIDEOSYNCHRONIZATION_FFMPEGHELPER_H
#define AUDIOVIDEOSYNCHRONIZATION_FFMPEGHELPER_H

extern "C" {
//工具库
#include "libavutil/avutil.h"
//封装格式处理库
#include "libavformat/avformat.h"
//编码解码库
#include "libavcodec/avcodec.h"
//音频采样数据格式转换库
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JavaCallHelper.h"


class FFmpegHelper {


public:
    FFmpegHelper(JavaCallHelper *javaCallHelper, const char *url);

    ~FFmpegHelper();

    void setRenderFrameCallBack(VideoChannel::RenderFrame callBack);


    void prepare();

    void prepareFFmpegInit();

    void start();

    void startDecode();

private:
    void getCurrentDecode(AVCodecContext *AvCtx,int stream_index);
    JavaCallHelper *javaCallHelper;
    const char *url;
    AVCodecContext *videoAVCtx;
    AVCodecContext *audioAVCtx;
    AVFormatContext *pAVFContext;
    AudioChannel *audioChannel;
    VideoChannel *videoChannel;
    pthread_t prepareThread;
    pthread_t playThread;
    bool isPlaying = false;
};


#endif //AUDIOVIDEOSYNCHRONIZATION_FFMPEGHELPER_H
