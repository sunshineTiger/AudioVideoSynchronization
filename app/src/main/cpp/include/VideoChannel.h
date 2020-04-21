//
// Created by zh on 2020/4/13.
//

#ifndef AUDIOVIDEOSYNCHRONIZATION_VIDEOCHANNEL_H
#define AUDIOVIDEOSYNCHRONIZATION_VIDEOCHANNEL_H


extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include "pthread.h"
#include "JavaCallHelper.h"
#include "safe_queue.h"
#include "AudioChannel.h"
class VideoChannel {
public:
    typedef void (*RenderFrame)(uint8_t *data, int lineSize, int w, int h);

    VideoChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                 AVRational time_base,AudioChannel *audioChannel);

    ~VideoChannel();

    void decodePackets();

    void goPlayFrame();

    void setRenderFrameCallBack(RenderFrame renderFrame);

    void play();

    void stop();

    void setFps(int fps);

    int channelId;
    bool isPlaying = false;
    SafeQueue<AVPacket *> pkt_queue;
    SafeQueue<AVFrame *> frame_queue;
private:
    AudioChannel *audioChannel;
    RenderFrame renderFrame;
    double clock;
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    AVRational time_base;
    int id;
    int fps;
    JavaCallHelper *javaCallHelper;
    AVCodecContext *avCodecContext;
};


#endif //AUDIOVIDEOSYNCHRONIZATION_VIDEOCHANNEL_H
