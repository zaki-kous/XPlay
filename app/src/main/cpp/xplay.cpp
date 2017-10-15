#include <jni.h>
#include <string>
#include "log.h"

#include "xplay.h"
double r2d(AVRational rational);

extern "C"
JNIEXPORT jint JNICALL
Java_com_me_xplay_video_XPlayer_openVideo(
        JNIEnv *env,
        jobject obj, jstring jpath) {
    const char* path = env->GetStringUTFChars(jpath, NULL);
    XPlay *play = XPlay::GetXPlay();
    bool ret = play->Open(path);
    if (!ret) {
        LOGE("open video path[%s] fail[%s].", path, play->GetError().c_str());
        return -1;
    }
    LOGI("open video path[%s] success , duration[%d]", path, play->GetDuration());

    int outWidth = 640;
    int outheight = 480;
    uint8_t *rgb = new uint8_t[outheight * outWidth * 4];
    for (;;) {
        AVPacket packet = play->ReadPacket();
        if (packet.size == 0) {
            break;
        }
        if (packet.stream_index != play->GetVideoStream()) { //only deal video stream
            av_packet_unref(&packet);
            continue;
        }
        AVFrame *yuvPicture = play->Decode(&packet);
        if (!yuvPicture) {
            av_packet_unref(&packet);
            LOGE("Decode fail[%s]", play->GetError().c_str());
            continue;
        }
        if(play->ToRGB(yuvPicture, rgb, outWidth, outheight)) {
            LOGI("ToRGB success.");
        }
        av_packet_unref(&packet);
    }
    delete rgb;
//    SwsContext *swsCtx = NULL;
//    int outWidth = 640;
//    int outheight = 480;
//    AVFrame *yuvPicture = av_frame_alloc();
//    int i = 0;
//
//    uint8_t *rgb = new uint8_t[outheight * outWidth * 4];
//    for (;;) {
//        AVPacket packet;
//        ret = av_read_frame(ic, &packet);
//        if (ret != 0) break;
//
//        if (packet.stream_index != videoStream) {
//            av_packet_unref(&packet);
//            LOGI("stream index[%d] don't video ", packet.stream_index);
//            continue;
//        }
////        int got_picture = 0;
////        ret = avcodec_decode_video2(videoCtx, yuvPicture, &got_picture, &packet);
////        if (got_picture) {
////            LOGI("avcodec_decode_video sucess ret[%d]", ret);
////        }
//        ret = avcodec_send_packet(videoCtx, &packet);
//        if (ret != 0) {
//            LOGE("avcodec_send_packet fail[%d]", ret);
//            av_packet_unref(&packet);
//            continue;
//        }
//
//        ret = avcodec_receive_frame(videoCtx, yuvPicture);
//        if (ret != 0) {
//            LOGE("avcodec_receive_frame fail[%d]", ret);
//            av_packet_unref(&packet);
//            continue;
//        }
//
//        int pts = (int) (packet.pts * av_q2d(ic->streams[packet.stream_index]->time_base) * 1000);
//        int dts = (int) (packet.dts * av_q2d(ic->streams[packet.stream_index]->time_base) * 1000);
//        if (i < 100) {
//            LOGI("avcodec decode frame success width [%d] height[%d]", yuvPicture->width, yuvPicture->height);
//            LOGI("read frame pts[%d] dts[%d] frameType[%d]", pts, dts, yuvPicture->pict_type);
//        }
//        // sws------
//        swsCtx = sws_getCachedContext(swsCtx, videoCtx->width, videoCtx->height, videoCtx->pix_fmt,
//                                      outWidth, outheight, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);
//        if (!swsCtx) {
//            LOGE("");
//            break;
//        }
//        LOGI("get SwsContext success [%p]", swsCtx);
//
//        uint8_t *outData[AV_NUM_DATA_POINTERS] = {0};
//        outData[0] = rgb;
//        int linesize[AV_NUM_DATA_POINTERS] = {0};
//        linesize[0] = outWidth * 4;
//        int h = sws_scale(swsCtx, yuvPicture->data, yuvPicture->linesize, 0, videoCtx->height, outData, linesize);
//        if (h > 0) {
//            LOGI("sws_scale success[%d]", h);
//        }
//
//        av_packet_unref(&packet);
//        i++;
//    }
//
//    if (swsCtx) {
//        sws_freeContext(swsCtx);
//        swsCtx = NULL;
//    }
//    avformat_close_input(&ic);

    return 0;
}

double r2d(AVRational rational) {
    return rational.num == 0 || rational.den == 0 ? 0 : (double) rational.num / (double) rational.den;
}

XPlay::XPlay() : totalDuration(0), videoStream(0), ic(NULL), swsCtx(NULL), yuvPicture(NULL){
    pthread_mutex_init(&mutex, NULL); //初始化锁对象
    av_register_all(); //注册ffmpeg
}

XPlay::~XPlay(){

}

bool XPlay::Open(const char *path) {
    Close();

    pthread_mutex_lock(&mutex);
    int ret = avformat_open_input(&ic, path, 0, 0);
    if (ret != 0) {
        av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
        pthread_mutex_unlock(&mutex);
        return false;
    }
    totalDuration = (ic->duration / AV_TIME_BASE) * 1000;
    for (int i = 0; i < ic->nb_streams; i++) {
        AVCodecContext *codecContext = ic->streams[i]->codec;
        if (codecContext->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
            if (!codec) {
                av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
                pthread_mutex_unlock(&mutex);
                return false;
            }

            ret = avcodec_open2(codecContext, codec, NULL);
            if (ret != 0) {
                av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
                pthread_mutex_unlock(&mutex);
                return false;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return true;
}

void XPlay::Close() {
    pthread_mutex_lock(&mutex);
    if (ic) {
        avformat_close_input(&ic);
        ic = NULL;
    }

    if (yuvPicture) {
        av_frame_free(&yuvPicture);
    }

    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = NULL;
    }
    pthread_mutex_unlock(&mutex);
}

AVPacket XPlay::ReadPacket() {
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));
    pthread_mutex_lock(&mutex);
    if (!ic) {
        pthread_mutex_unlock(&mutex);
        return pkt;
    }
    int ret = av_read_frame(ic, &pkt);
    if (ret != 0) {
        av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
    }
    pthread_mutex_unlock(&mutex);
    return pkt;
}

AVFrame* XPlay::Decode(const AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    if (!ic){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    if (yuvPicture == NULL) {
        yuvPicture = av_frame_alloc();
    }
    int ret = avcodec_send_packet(ic->streams[packet->stream_index]->codec, packet);
    if (ret != 0) {
        av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    ret = avcodec_receive_frame(ic->streams[packet->stream_index]->codec, yuvPicture);
    if (ret != 0) {
        av_strerror(ret, av_error_buffer, sizeof(av_error_buffer));
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);
    return yuvPicture;
}

bool XPlay::ToRGB(const AVFrame *yuvPicture, uint8_t *out, int outWidth, int outHeight) {
    pthread_mutex_lock(&mutex);
    if (!ic) {
        pthread_mutex_unlock(&mutex);
        return false;
    }

    AVCodecContext *videoCtx = ic->streams[videoStream]->codec;
    swsCtx = sws_getCachedContext(swsCtx, videoCtx->width, videoCtx->height, videoCtx->pix_fmt,
                                      outWidth, outHeight, AV_PIX_FMT_BGRA, SWS_BICUBIC, NULL, NULL, NULL);
    if (!swsCtx) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    uint8_t *outData[AV_NUM_DATA_POINTERS] = {0};
    outData[0] = out;
    int linesize[AV_NUM_DATA_POINTERS] = {0};
    linesize[0] = outWidth * 4;
    int h = sws_scale(swsCtx, yuvPicture->data, yuvPicture->linesize, 0, videoCtx->height, outData, linesize);
    if (h != outHeight) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    pthread_mutex_unlock(&mutex);
    return true;
}

std::string XPlay::GetError() {
    pthread_mutex_lock(&mutex);
    std::string ret = this->av_error_buffer;
    pthread_mutex_unlock(&mutex);
    return ret;
}

int XPlay::GetDuration() const {
    return totalDuration;
}

int XPlay::GetVideoStream() const {
    return videoStream;
}
