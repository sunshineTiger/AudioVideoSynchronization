//
// Created by zh on 2020/4/13.
//


#include "AudioChannel.h"

int i = 0;

void *decodeAudio(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decodePackets();
    return 0;
}

void *initOpenSLES(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->InitOpenSLES();
    return 0;
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //获取pcm缓冲队列大小
    int dataLength = audioChannel->changeFrameToPcm();
    if (dataLength > 0) {
        //送入音频播放队列播放 喇叭会从队列中主动去拿这些数据进行播放
        (*bq)->Enqueue(bq, audioChannel->out_buffer, dataLength);
    }
}

AudioChannel::AudioChannel(int id, JavaCallHelper *javaCallHelper,
                           AVCodecContext *avCodecContext, AVRational time_base) : channelId(id),
                                                                                   javaCallHelper(
                                                                                           javaCallHelper),
                                                                                   avCodecContext(
                                                                                           avCodecContext) {
    this->time_base = time_base;
    //输出布局声道
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //输出采样率格式
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输出采样率
    out_sample_rate = 44100;
    out_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);
    //获取声道布局
    //根据声道个数默认的声道布局(两个声道，默认立体声)
    uint64_t in_ch_layout = avCodecContext->channel_layout;
    //输入采样率格式
    enum AVSampleFormat in_sample_fmt = avCodecContext->sample_fmt;
    //输入采样率
    int in_sample_rate = avCodecContext->sample_rate;
    swr_ctx = swr_alloc_set_opts(0, out_ch_layout, out_sample_fmt, out_sample_rate,
                                 in_ch_layout,
                                 in_sample_fmt,
                                 in_sample_rate, 0, 0);
    swr_init(swr_ctx);
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    pthread_create(&pid_init_openSLES, NULL, initOpenSLES, this);
}


void AudioChannel::play() {
    isPlaying = true;
    pthread_create(&pid_audio_decode, NULL, decodeAudio, this);

}

void AudioChannel::InitOpenSLES() {
//音频引擎
    SLEngineItf engineInterface = NULL;
    //音频对此昂
    SLObjectItf engineObject = NULL;
    //混音器
    SLObjectItf outputMixObject = NULL;

    //播放器
    SLObjectItf bqPlayerObject = NULL;
//    回调接口
    SLPlayItf bqPlayerInterface = NULL;
//    缓冲队列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    //todo 第一步、初始化播放引擎
    SLresult result;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //音频接口  相当于SurfaceHodler
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    //todo 第二步、设置混音器
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    // 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //todo 第三步、创建播放器
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM//播放pcm格式的数据
            , 2,//2个声道（立体声）
                            SL_SAMPLINGRATE_44_1, //44100hz的频率
                            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
                            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
                            SL_BYTEORDER_LITTLEENDIAN//小端模式
    };
    //
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    SLDataSource slDataSource = {&android_queue, &pcm};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject// //播放器
            , &slDataSource//播放器参数  播放缓冲队列   播放格式
            , &audioSnk,//播放缓冲区
                                          1,//播放接口回调个数
                                          ids,//设置播放队列ID
                                          req//是否采用内置的播放队列
    );
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //todo 第四步、设置缓存队列和回调函数
//    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
//    获得播放器接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    // todo 第五步、设置播放状态，需要手动进行设置
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    //todo 第六步、启动回调函数 只要启动就会不断的被回调 把pcm数据缓存到缓冲区中
    bqPlayerCallback(bqPlayerBufferQueue, this);
    LOGE("--- 手动调用播放 packet:%d", this->pkt_queue.size());
}


void AudioChannel::decodePackets() {

    AVPacket *packet = 0;

    while (isPlaying) {
        //从队列中取一个
        int ret = pkt_queue.get(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        ret = avcodec_send_packet(avCodecContext, packet);
        av_packet_unref(packet);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret < 0) {
            //失败
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret < 0) {
            break;
        }
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
        //packet包解码后放入帧队列
        frame_queue.put(frame);
    }
}

int AudioChannel::changeFrameToPcm() {
    //播放的时候才需要转换
    AVFrame *frame = 0;
    int data_size = 0;

    while (isPlaying) {
        int ret = frame_queue.get(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 转换，返回值为转换后的sample个数  buffer malloc（size） 转换的数据放到buffer中
        swr_convert(swr_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE,
                    (const uint8_t **) frame->data, frame->nb_samples);
        //计算buffer大小
        data_size = av_samples_get_buffer_size(NULL, out_channels,
                                               frame->nb_samples, out_sample_fmt, 1);
        //数量*单位 pts是数量
        clock = frame->pts * av_q2d(time_base);
        break;
    }
    if (frame)
        av_frame_free(&frame);
    return data_size;

}

void AudioChannel::stop() {}