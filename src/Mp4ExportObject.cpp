
#include "Mp4ExportObject.h"

#include <cstdio>
#include <cmath>
extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}
#include "fractionApprox.h"

struct Mp4ExportData {
    AVRational timeBase;
    AVCodecID codecId;
    AVPixelFormat pixFmt;
    AVFrame *frame;
    AVFormatContext *fc;
    AVCodecContext *cc;
    AVIOContext *ioc;
    AVStream *stream;
    SwsContext *sc;
};

Mp4ExportObject::Mp4ExportObject(int sourceId, const std::string &filename, Codec codec, PixelFormat pixelFormat, const std::string &settings, float framerate, float duration, const LogicalObject *framerateSource, const LogicalObject *durationSource) : LogicalObject(std::string()), data(new Mp4ExportData), sourceId(sourceId), filename(filename), codec(codec), pixelFormat(pixelFormat), settings(settings), framerate(framerate), duration(duration), framerateSource(framerateSource), durationSource(durationSource) {
    step = -1;
    width = 0, height = 0;
    frameCount = (int) ceilf(framerate*duration);
    if (framerate != 0.f) {
        frameDuration = 1.f/framerate;
        fractionApprox(data->timeBase.den, data->timeBase.num, framerate, 1024);
    } else {
        frameDuration = 0.f;
        data->timeBase.num = 0;
        data->timeBase.den = 1;
    }
    switch (codec) {
        case H264:
            data->codecId = AV_CODEC_ID_H264;
            break;
        case HEVC:
            data->codecId = AV_CODEC_ID_HEVC;
            break;
        default:
            data->codecId = AV_CODEC_ID_NONE;
    }
    switch (pixelFormat) {
        case YUV420:
            data->pixFmt = AV_PIX_FMT_YUV420P;
            break;
        case YUV444:
            data->pixFmt = AV_PIX_FMT_YUV444P;
            break;
        default:
            data->pixFmt = AV_PIX_FMT_NONE;
    }
    data->frame = av_frame_alloc();
    data->fc = NULL;
    data->cc = NULL;
    data->ioc = NULL;
    data->stream = NULL;
    data->sc = NULL;
}

Mp4ExportObject::~Mp4ExportObject() {
    finishExport();
    if (data->frame)
        av_frame_free(&data->frame);
    delete data;
}

bool Mp4ExportObject::offerSource(int sourceId) const {
    return sourceId == this->sourceId;
}

void Mp4ExportObject::setSourcePixels(int sourceId, const void *pixels, int width, int height) {
    if (sourceId == this->sourceId && data->frame && step >= 0) {
        if (step == 0) {
            data->frame->format = data->pixFmt;
            data->frame->width = width;
            data->frame->height = height;
            if (av_frame_get_buffer(data->frame, 32) >= 0) {
                if ((data->sc = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, data->pixFmt, 0, NULL, NULL, NULL))) {
                    this->width = width;
                    this->height = height;
                }
            }
        }
        if (width == this->width && height == this->height && data->sc) {
            const uint8_t *invImgData[4] = { reinterpret_cast<const uint8_t *>(pixels)+4*width*(height-1) };
            int invImgLinesizes[4] = { -4*width };
            sws_scale(data->sc, invImgData, invImgLinesizes, 0, height, data->frame->data, data->frame->linesize);
        }
    }
}

bool Mp4ExportObject::startExport() {
    if (data->frame && !data->fc) {
        if (framerateSource) {
            if (!framerateSource->getFramerate(data->timeBase.den, data->timeBase.num))
                return false;
            framerate = (float) data->timeBase.den/data->timeBase.num;
            frameDuration = (float) data->timeBase.num/data->timeBase.den;
        }
        if (durationSource) {
            if (!durationSource->getDuration(duration))
                return false;
        }
        frameCount = (int) ceilf(framerate*duration);
        if (avformat_alloc_output_context2(&data->fc, NULL, "mp4", NULL) >= 0) {
            data->stream = avformat_new_stream(data->fc, NULL);
            if (data->stream) {
                data->stream->time_base = data->timeBase;
                return true;
            }
            avformat_free_context(data->fc);
            data->fc = NULL;
        }
    }
    return false;
}

void Mp4ExportObject::finishExport() {
    if (data->sc) {
        sws_freeContext(data->sc);
        data->sc = NULL;
    }
    data->stream = NULL;
    if (data->cc) {
        avcodec_close(data->cc);
        avcodec_free_context(&data->cc);
    }
    if (data->ioc)
        avio_closep(&data->ioc);
    if (data->fc) {
        avformat_free_context(data->fc);
        data->fc = NULL;
    }
}

int Mp4ExportObject::getExportStepCount() const {
    return frameCount;
}

std::string Mp4ExportObject::getExportFilename() const {
    return filename;
}

bool Mp4ExportObject::prepareExportStep(int step, float &time, float &deltaTime) {
    time = step/framerate;
    deltaTime = frameDuration;
    this->step = step;
    return true;
}

bool Mp4ExportObject::exportStep() {
    if (!(data->frame && data->fc && data->stream && step >= 0 && step < frameCount))
        return false;
    if (step == 0) {
        if (pixelFormat == YUV420 && (width&1 || height&1))
            return false;
        if (avio_open2(&data->fc->pb, filename.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0)
            return false;
        data->ioc = data->fc->pb;
        AVCodec *codec = avcodec_find_encoder(data->codecId);
        if (!codec)
            return false;
        data->cc = avcodec_alloc_context3(codec);
        data->cc->codec_type = AVMEDIA_TYPE_VIDEO;
        data->cc->width = width;
        data->cc->height = height;
        data->cc->sample_aspect_ratio.num = 1;
        data->cc->sample_aspect_ratio.den = 1;
        data->cc->time_base = data->timeBase;
        data->cc->pix_fmt = data->pixFmt;
        data->cc->framerate.num = data->timeBase.den;
        data->cc->framerate.den = data->timeBase.num;
        if (data->fc->oformat->flags&AVFMT_GLOBALHEADER)
            data->cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        AVDictionary *options = NULL;
        av_dict_parse_string(&options, settings.c_str(), "=", ",", 0);
        if (avcodec_open2(data->cc, codec, &options) < 0) {
            avcodec_free_context(&data->cc);
            av_dict_free(&options);
            return false;
        }
        av_dict_free(&options);
        if (avcodec_parameters_from_context(data->stream->codecpar, data->cc) < 0 || avformat_write_header(data->fc, NULL) < 0) {
            avcodec_free_context(&data->cc);
            return false;
        }
    }
    if (!data->cc)
        return false;
    {
        data->frame->pts = step;
        if (avcodec_send_frame(data->cc, data->frame) != 0)
            return false;
        AVPacket pkt = { };
        av_init_packet(&pkt);
        while (avcodec_receive_packet(data->cc, &pkt) == 0) {
            pkt.stream_index = data->stream->index;
            av_packet_rescale_ts(&pkt, data->timeBase, data->stream->time_base);
            if (av_interleaved_write_frame(data->fc, &pkt) != 0) {
                av_packet_unref(&pkt);
                return false;
            }
            av_packet_unref(&pkt);
        }
        if (step == frameCount-1) {
            if (avcodec_send_frame(data->cc, NULL) != 0)
                return false;
            while (avcodec_receive_packet(data->cc, &pkt) == 0) {
                pkt.stream_index = data->stream->index;
                av_packet_rescale_ts(&pkt, data->timeBase, data->stream->time_base);
                if (av_interleaved_write_frame(data->fc, &pkt) != 0) {
                    av_packet_unref(&pkt);
                    return false;
                }
                av_packet_unref(&pkt);
            }
            av_write_trailer(data->fc);
        }
    }
    return true;
}
