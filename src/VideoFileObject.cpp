
#include "VideoFileObject.h"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

struct VideoFileData {
    AVFrame *frame;
    AVFormatContext *fc;
    AVCodecContext *cc;
    int streamId;
    AVRational timeBase;
    uint8_t *imgData[4];
    int imgLinesizes[4];
    uint8_t *invImgData[4];
    int invImgLinesizes[4];
    SwsContext *sc;
};

VideoFileObject::VideoFileObject(const std::string &name, const std::string &filename) : LogicalObject(name), data(new VideoFileData), initialFilename(filename) {
    prepared = false;
    fileOpen = false;
    width = 0, height = 0;
    repeat = false;
    atStart = false;
    atLastFrame = false;
    frameEndTime = 0;
    frameRemainingTime = 0;
    data->frame = av_frame_alloc();
    data->fc = NULL;
    data->cc = NULL;
    data->streamId = 0;
    memset(data->imgData, 0, sizeof(data->imgData));
    memset(data->imgLinesizes, 0, sizeof(data->imgLinesizes));
    memset(data->imgData, 0, sizeof(data->invImgData));
    memset(data->imgLinesizes, 0, sizeof(data->invImgLinesizes));
    data->sc = NULL;
}

VideoFileObject::~VideoFileObject() {
    unloadFile();
    if (data->frame)
        av_frame_free(&data->frame);
    delete data;
}

bool VideoFileObject::prepare(int &width, int &height, bool hardReset, bool repeat) {
    this->repeat = repeat;
    if (!prepared || hardReset) {
        if (!initialFilename.empty())
            loadFile(initialFilename.c_str());
        else
            unloadFile();
    }
    return prepared = getSize(width, height);
}

bool VideoFileObject::getSize(int &width, int &height) const {
    width = this->width;
    height = this->height;
    return true;
}

bool VideoFileObject::getFramerate(int &num, int &den) const {
    if (fileOpen) {
        num = data->fc->streams[data->streamId]->r_frame_rate.num;
        den = data->fc->streams[data->streamId]->r_frame_rate.den;
        return true;
    }
    return false;
}

bool VideoFileObject::getDuration(float &duration) const {
    if (fileOpen) {
        duration = (float) data->fc->duration/AV_TIME_BASE;
        return true;
    }
    return false;
}

bool VideoFileObject::acceptsFiles() const {
    return true;
}

bool VideoFileObject::loadFile(const char *filename) {
    if (!filename) {
        if (initialFilename.empty())
            return false;
        filename = initialFilename.c_str();
    }
    AVFormatContext *fc = NULL;
    if (data->frame) {
        if (avformat_open_input(&fc, filename, NULL, NULL) >= 0) {
            if (avformat_find_stream_info(fc, NULL) >= 0) {
                AVCodec *decoder = NULL;
                int streamId = av_find_best_stream(fc, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
                if (streamId >= 0 && decoder) {
                    AVCodecContext *cc = avcodec_alloc_context3(decoder);
                    if (cc) {
                        const AVCodecParameters *codecpar = fc->streams[streamId]->codecpar;
                        if (avcodec_parameters_to_context(cc, codecpar) >= 0) {
                            AVDictionary *options = NULL;
                            if (avcodec_open2(cc, decoder, &options) >= 0) {
                                SwsContext *sc = sws_getContext(cc->width, cc->height, cc->pix_fmt, cc->width, cc->height, AV_PIX_FMT_RGBA, 0, NULL, NULL, NULL);
                                if (sc) {
                                    uint8_t *imgData[4] = { };
                                    int imgLinesizes[4] = { };
                                    int bitmapSize = av_image_alloc(imgData, imgLinesizes, cc->width, cc->height, AV_PIX_FMT_RGBA, 1);
                                    if (bitmapSize >= 0) {
                                        unloadFile();
                                        data->fc = fc;
                                        data->cc = cc;
                                        data->streamId = streamId;
                                        data->timeBase = fc->streams[streamId]->time_base;
                                        for (int i = 0; i < 4; ++i) {
                                            data->imgData[i] = imgData[i];
                                            data->imgLinesizes[i] = imgLinesizes[i];
                                            data->invImgData[i] = imgData[i]+imgLinesizes[i]*(cc->height-1);
                                            data->invImgLinesizes[i] = -imgLinesizes[i];
                                        }
                                        data->sc = sc;
                                        width = cc->width;
                                        height = cc->height;
                                        fileOpen = true;
                                        atStart = true;
                                        atLastFrame = false;
                                        frameEndTime = 0;
                                        frameRemainingTime = 0;
                                        return true;
                                    }
                                    sws_freeContext(sc);
                                }
                                avcodec_close(cc);
                            }
                        }
                        avcodec_free_context(&cc);
                    }
                }
            }
            avformat_close_input(&fc);
        }
    }
    return false;
}

void VideoFileObject::unloadFile() {
    fileOpen = false;
    if (data->imgData[0])
        av_freep(&data->imgData[0]);
    if (data->sc) {
        sws_freeContext(data->sc);
        data->sc = NULL;
    }
    if (data->cc) {
        avcodec_close(data->cc);
        avcodec_free_context(&data->cc);
    }
    if (data->fc)
        avformat_close_input(&data->fc);
}

bool VideoFileObject::restart() {
    if (fileOpen && !atStart) {
        rewind();
        return true;
    }
    return false;
}

bool VideoFileObject::pixelsReady() const {
    return fileOpen;
}

bool VideoFileObject::rewind() {
    if (atStart)
        return true;
    avcodec_flush_buffers(data->cc);
    if (av_seek_frame(data->fc, -1, data->fc->start_time, 0) < 0)
        return false;
    atStart = true;
    atLastFrame = false;
    frameEndTime = 0;
    frameRemainingTime = 0;
    return true;
}

bool VideoFileObject::nextFrame() {
    bool prevAtStart = atStart;
    atStart = false;
    if (!avcodec_receive_frame(data->cc, data->frame))
        return true;
    AVPacket pkt = { };
    av_init_packet(&pkt);
    while (av_read_frame(data->fc, &pkt) == 0) {
        if (pkt.stream_index == data->streamId) {
            if (avcodec_send_packet(data->cc, &pkt) < 0) {
                av_packet_unref(&pkt);
                return false;
            }
            if (!avcodec_receive_frame(data->cc, data->frame)) {
                av_packet_unref(&pkt);
                return true;
            }
        }
        av_packet_unref(&pkt);
    }
    if (avcodec_send_packet(data->cc, NULL) < 0)
        return false;
    if (!avcodec_receive_frame(data->cc, data->frame))
        return true;
    if (repeat && !prevAtStart)
        return rewind() && nextFrame();
    atLastFrame = true;
    return false;
}

bool VideoFileObject::isFrameCurrent(float time, bool realTime) {
    if (realTime) {
        return frameRemainingTime > 0.0;
    } else {
        double frameEndRealTime = (double) frameEndTime*data->timeBase.num/data->timeBase.den;
        return frameEndRealTime > (double) time;
    }
}

const void * VideoFileObject::fetchPixels(float time, float deltaTime, bool realTime, int width, int height) {
    if (fileOpen && width == this->width && height == this->height) {
        if (time == 0.f)
            rewind();
        frameRemainingTime -= (double) deltaTime;
        if (atLastFrame || (!atStart && isFrameCurrent(time, realTime)))
            return NULL;
        do {
            if (!nextFrame())
                return NULL;
            int64_t frameDuration = data->frame->pkt_duration;
            frameEndTime += frameDuration;
            frameRemainingTime += (double) frameDuration*data->timeBase.num/data->timeBase.den;
        } while (!isFrameCurrent(time, realTime));
        sws_scale(data->sc, data->frame->data, data->frame->linesize, 0, height, data->invImgData, data->invImgLinesizes);
        return data->imgData[0];
    }
    return NULL;
}
