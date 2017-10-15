//
// Created by 朱乾 on 17/10/14.
//
#ifndef XPLAY_XPLAY_H
#define XPLAY_XPLAY_H

#include <string.h>
#include <pthread.h>

extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

class XPlay {
public:
    static XPlay* GetXPlay() {
        static XPlay xPlay;
        return &xPlay;
    }

    // Open video .
    bool Open(const char* path);
    void Close();

    AVPacket ReadPacket();
    AVFrame *Decode(const AVPacket *packet);
    bool ToRGB(const AVFrame *yuvPicture, uint8_t *out, int outWidth, int outHeight);

    std::string GetError();

    virtual ~XPlay();

    int GetDuration() const;
    int GetVideoStream() const;

private:
    pthread_mutex_t mutex;
    AVFormatContext *ic;
    AVFrame *yuvPicture;
    SwsContext *swsCtx;
    int totalDuration;
    int videoStream;

    char av_error_buffer[1024];
protected:
    XPlay();
};

#endif //XPLAY_XPLAY_H
