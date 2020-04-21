//
// Created by zh on 2020/4/13.
//

#ifndef AUDIOVIDEOSYNCHRONIZATION_AUDIOCHANNEL_H
#define AUDIOVIDEOSYNCHRONIZATION_AUDIOCHANNEL_H


#include "safe_queue.h"
#include "JavaCallHelper.h"

extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}
#define MAX_AUDIO_FRAME_SIZE  2 * 44100

class AudioChannel {

public:

    AudioChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                 AVRational time_base);

    void play();

    void stop();

    void decodePackets();

    int changeFrameToPcm();

    void InitOpenSLES();

    int channelId;
    bool isPlaying;
    SafeQueue<AVPacket *> pkt_queue;
    SafeQueue<AVFrame *> frame_queue;
    uint8_t *out_buffer;
    double clock;
private:

    AVRational time_base;
    int out_channels;
    enum AVSampleFormat out_sample_fmt;
    pthread_t pid_init_openSLES;
    pthread_t pid_audio_decode;
    JavaCallHelper *javaCallHelper;
    SwrContext *swr_ctx;
    AVCodecContext *avCodecContext;
    int out_sample_rate;

};


#endif //AUDIOVIDEOSYNCHRONIZATION_AUDIOCHANNEL_H
