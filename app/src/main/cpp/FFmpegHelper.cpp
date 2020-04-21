//
// Created by zh on 2020/4/13.
//

#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

#include "FFmpegHelper.h"


/**
 * 线程执行回调方法 一定要返回0
 */
void *prepareFFmpeg(void *args) {
    //强转对象，拿到当前外部操作对象
    FFmpegHelper *fFmpeg = static_cast<FFmpegHelper *>(args);
    fFmpeg->prepareFFmpegInit();
    return 0;

}

void *play(void *args) {
    FFmpegHelper *fFmpeg = static_cast<FFmpegHelper *>(args);
    fFmpeg->startDecode();
    return 0;
}

void FFmpegHelper::start() {
    isPlaying = true;
    if (videoChannel) {
        videoChannel->play();
    }
    //音频开始播放 交给音频对象处理
    if (audioChannel) {
        audioChannel->play();
    }

    pthread_create(&playThread, NULL, play, this);
};

FFmpegHelper::FFmpegHelper(JavaCallHelper *javaCallHelper, const char *url) {
    this->url = url;
    this->javaCallHelper = javaCallHelper;
}


void FFmpegHelper::setRenderFrameCallBack(VideoChannel::RenderFrame callBack) {
    if (videoChannel)
        videoChannel->setRenderFrameCallBack(callBack);
}

void FFmpegHelper::prepare() {
    pthread_create(&prepareThread, NULL, prepareFFmpeg, this);
}

void FFmpegHelper::prepareFFmpegInit() {
/**
     * 第一步
     * 初始化网络组件
     * **/
    avformat_network_init();
    AVDictionary *options = NULL;
    av_dict_set(&options, "stimeout", "20000000", 0);
    /**
     * 第二步
     * 打开封装格式->打开文件
     * AVFormatContext：作用保存整个视频信息（解码器，编码器）
     * 信息：码率，帧率等
     * **/
    //获取AVFormat上下文
    pAVFContext = avformat_alloc_context();
    /**
    * 指定输入的参宿
    * 设置默认参数 AVDictionary* options = NULL;
    * **/
    if (!pAVFContext) {
        LOGE("不能打开指定文件");
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    int ret = avformat_open_input(&pAVFContext, url, NULL, NULL);
    if (ret != 0) {
        LOGE("不能打开指定文件");
        if (pAVFContext) {
            //释放上下文
            avformat_free_context(pAVFContext);
        }
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    /**
    * 第三步
    * 获取视频信息
    * **/
    ret = avformat_find_stream_info(pAVFContext, NULL);
    if (ret < 0) {
        LOGE("查找视频信息失败");
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    /**
   * 第四步
   * 查找视频编码器
   * **/
    /**1.查找视频流位置索引**/
    int video_stream_index = -1;
    int audio_stream_index = -1;
    bool foundVideo = false;
    bool foundAudio = false;
    for (int i = 0; i < pAVFContext->nb_streams; ++i) {
        if (!foundVideo && pAVFContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            foundVideo = true;
            video_stream_index = i;
        }
        if (!foundAudio && pAVFContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            foundAudio = true;
            audio_stream_index = i;
        }
        if (foundVideo && foundAudio)
            break;
    }
    getCurrentDecode(audioAVCtx, audio_stream_index);
    getCurrentDecode(videoAVCtx, video_stream_index);

    if (!videoChannel && !audioChannel) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    //表示ffmpeg初始化准备好了
    if (javaCallHelper) {
        javaCallHelper->onPrepare(THREAD_CHILD);
    }
}


void FFmpegHelper::getCurrentDecode(AVCodecContext *AvCtx, int stream_index) {
    /**2.根据索引值获取指定解码**/
    AVCodecParameters *pavCoderPar = pAVFContext->streams[stream_index]->codecpar;
    /**根据解码器上下文，获得解码器ID，根据ID查找解码器**/
    AVCodec *pAVCoder = avcodec_find_decoder(pavCoderPar->codec_id);
    if (!pAVCoder) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
        }
    }
    AvCtx = avcodec_alloc_context3(pAVCoder);
    if (!AvCtx) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
        }
    }
    if (avcodec_parameters_to_context(AvCtx, pavCoderPar) < 0) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
        }
    }
    /**
     * 第五步
     * 打开解码器
     * **/
    int ret = avcodec_open2(AvCtx, pAVCoder, NULL);
    if (ret != 0) {
        LOGE("打开解码器失败");
        if (AvCtx) {
            avcodec_free_context(&AvCtx);
        }
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
        }
        return;
    }
    //输出信息
    LOGI("视频的文件格式：%s", pAVFContext->iformat->name);
    LOGI("视频时长：%lld", (pAVFContext->duration) / (1000 * 1000));
    LOGI("视频的宽高：%d,%d", AvCtx->width, AvCtx->height);
    LOGI("解码器的名称：%s", pAVCoder->name);
    if (pavCoderPar->codec_type == AVMEDIA_TYPE_VIDEO) {
        AVRational frame_rate = pAVFContext->streams[stream_index]->avg_frame_rate;
        int fps = av_q2d(frame_rate);
        videoChannel = new VideoChannel(stream_index, javaCallHelper, AvCtx,
                                        pAVFContext->streams[stream_index]->time_base,
                                        audioChannel);
        videoChannel->setFps(fps);
    } else {

        audioChannel = new AudioChannel(stream_index, javaCallHelper, AvCtx,
                                        pAVFContext->streams[stream_index]->time_base);
    }
}

void FFmpegHelper::startDecode() {
    int ret = 0;
    while (isPlaying) {
        if (audioChannel && audioChannel->pkt_queue.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }
        if (videoChannel && videoChannel->pkt_queue.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }
        AVPacket *avPacket = av_packet_alloc();//手动管理
        ret = av_read_frame(pAVFContext, avPacket);
        if (ret == 0) {
            //将数据包加入队列
            if (audioChannel && avPacket->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.put(avPacket);
            } else if (videoChannel && avPacket->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.put(avPacket);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完毕了，但不一定是播放完毕
            //videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty() &&
            if (
                    audioChannel->pkt_queue.empty() && audioChannel->frame_queue.empty()) {
                break;
            }

        } else {
            break;
        }
    }
    isPlaying = false;
    videoChannel->isPlaying = false;
    audioChannel->isPlaying = false;
    audioChannel->stop();
    videoChannel->stop();
}
