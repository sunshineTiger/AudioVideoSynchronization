//
// Created by zh on 2020/4/13.
//

#include "VideoChannel.h"


/**
* 解码线程
*/
void *decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePackets();
    return 0;
}


void dropFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        if (frame) {
            q.pop();
            av_frame_free(&frame);
            frame = NULL;
        }
    }
}


void *goPlay(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->goPlayFrame();
}

void releaseAvFrame(AVFrame **frame) {
    if (frame) {
        av_frame_free(frame);
        frame = NULL;
    }
}

VideoChannel::VideoChannel(int
                           id, JavaCallHelper *javaCallHelper,
                           AVCodecContext *avCodecContext, AVRational
                           time_base,
                           AudioChannel *audioChannel) {
    this->time_base = time_base;
    this->audioChannel = audioChannel;
    this->id = id;
    this->javaCallHelper = javaCallHelper;
    this->avCodecContext = avCodecContext;
    //设置丢帧策略
    frame_queue.setReleaseHandler(releaseAvFrame);
    frame_queue.setSyncHandle(dropFrame);
}

VideoChannel::~VideoChannel() {}

void VideoChannel::play() {
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;
    pthread_create(&pid_video_decode, NULL, decode, this);
    pthread_create(&pid_video_play, NULL, goPlay, this);

}

void VideoChannel::stop() {

}

void VideoChannel::decodePackets() {
    AVPacket *packet = 0;
    while (isPlaying) {
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
            continue;
        } else if (ret < 0) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        frame_queue.put(frame);
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
    }
    av_packet_free(&packet);
}

void VideoChannel::goPlayFrame() {
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, 0);
    uint8_t *dst_data[4];
    int dst_lineSize[4];
    av_image_alloc(dst_data, dst_lineSize, avCodecContext->width, avCodecContext->height,
                   AV_PIX_FMT_RGBA, 1);
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frame_queue.get(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0, frame->height, dst_data, dst_lineSize);
        renderFrame(dst_data[0], dst_lineSize[0], avCodecContext->width,
                    avCodecContext->height);
        //16ms
//        av_usleep(16 * 1000);
//帧率,解码速度,渲染速度都会影响音视频同步
        //根据帧率求得延迟时间
        double fps_delay = 1 / fps;
        //渲染时间
        double decode_delay = frame->repeat_pict / (2 * fps);
        //总的延迟时间 解码时间也要算进去 配置差的手机解码比较慢
        double totalDelay = fps_delay + decode_delay;
        //视频的相对时间
        clock = frame->pts * av_q2d(time_base);
        //音频的相对时间
        double audioClock = audioChannel->clock;
        //视频时间刻度-音频时间刻度
        double diff = clock - audioClock;
        LOGE("音视频时间差--%d", diff);
        if (clock > audioClock) {
            //视频超前 休眠时间长一点 不然会卡很久
            if (diff > 1) {
                //相差比较大，延迟时间加大,加快同步时间
                av_usleep((totalDelay * 2) * 1000000);
            } else {
                //相差较小
                av_usleep((totalDelay + diff) * 1000000);

            }

        } else {
            //视频延后 音频超前
            if (diff > 1) {
                //休眠
            } else if (diff >= 0.05) {
                //视频需要追赶，丢非关键帧 同步
                av_frame_unref(frame);
                //丢frame队列 调用该方法直接调用dropFrame()方法
                frame_queue.sync();
            } else {


            }
        }

        av_frame_unref(frame);
    }
    av_free(&dst_data[0]);
    isPlaying = false;
    av_frame_free(&frame);
    sws_freeContext(swsContext);
}

void VideoChannel::setFps(int fps) {
    this->fps = fps;
}

void VideoChannel::setRenderFrameCallBack(VideoChannel::RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}