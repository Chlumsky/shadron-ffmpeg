
#include "SoundDecoder.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
extern "C" {
    #include <libavutil/opt.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

#define SAMPLE_SIZE 4
#define BUFFER_SIZE 0x10000

static void storeSamples(SwrContext *sc, AVFrame *frame, std::vector<unsigned char> &storage, int &sampleCount) {
    int newSamples = frame->nb_samples;
    if (newSamples > 0) {
        int prevSize = (int) storage.size();
        storage.resize(storage.size()+SAMPLE_SIZE*newSamples);
        unsigned char *target[8] = { &storage[prevSize] };
        swr_convert(sc, target, newSamples, const_cast<const uint8_t **>(frame->data), newSamples);
        sampleCount += newSamples;
    }
}

SoundDecoder * SoundDecoder::decode(const void *data, int length) {
    struct DataContext {
        const unsigned char *data;
        int pos, length;

        static int read(void *context, uint8_t *buffer, int bufferSize) {
            DataContext *data = reinterpret_cast<DataContext *>(context);
            if (data->pos < data->length && bufferSize > 0) {
                int chunk = data->length-data->pos;
                if (chunk > bufferSize)
                    chunk = bufferSize;
                memcpy(buffer, data->data+data->pos, chunk);
                data->pos += chunk;
                return chunk;
            }
            return 0;
        }
        static int64_t seek(void *context, int64_t offset, int whence) {
            DataContext *data = reinterpret_cast<DataContext *>(context);
            if (whence&AVSEEK_SIZE)
                return data->length;
            int64_t newPos = data->pos;
            switch (whence&~AVSEEK_FORCE) {
                case SEEK_SET:
                    newPos = offset;
                    break;
                case SEEK_CUR:
                    newPos += offset;
                    break;
                case SEEK_END:
                    newPos = data->length+offset;
                    break;
                default:
                    return -1;
            }
            if (newPos < 0 || newPos > (int64_t) data->length)
                return -1;
            data->pos = (int) newPos;
            return 0;
        }
    } dataContext = { reinterpret_cast<const unsigned char *>(data), 0, length };

    SoundDecoder *output = NULL;
    AVFormatContext *fc = avformat_alloc_context();
    if (fc) {
        void *buffer = av_malloc(BUFFER_SIZE);
        AVIOContext *ioc = avio_alloc_context(reinterpret_cast<unsigned char *>(buffer), BUFFER_SIZE, 0, &dataContext, &DataContext::read, NULL, &DataContext::seek);
        if (ioc) {
            fc->pb = ioc;
            if (avformat_open_input(&fc, "", NULL, NULL) >= 0) {
                if (avformat_find_stream_info(fc, NULL) >= 0) {
                    AVCodec *decoder = NULL;
                    int streamId = av_find_best_stream(fc, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
                    if (streamId >= 0 && decoder) {
                        AVCodecContext *cc = avcodec_alloc_context3(decoder);
                        if (cc) {
                            const AVCodecParameters *codecpar = fc->streams[streamId]->codecpar;
                            if (avcodec_parameters_to_context(cc, codecpar) >= 0) {
                                AVDictionary *options = NULL;
                                if (avcodec_open2(cc, decoder, &options) >= 0) {
                                    SwrContext *sc = swr_alloc();
                                    if (sc) {
                                        av_opt_set_int(sc, "in_channel_layout", cc->channel_layout, 0);
                                        av_opt_set_int(sc, "in_sample_rate", cc->sample_rate, 0);
                                        av_opt_set_int(sc, "in_sample_fmt", cc->sample_fmt, 0);
                                        av_opt_set_int(sc, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                                        av_opt_set_int(sc, "out_sample_rate", cc->sample_rate, 0);
                                        av_opt_set_int(sc, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                                        if (swr_init(sc) >= 0) {
                                            AVFrame *frame = av_frame_alloc();
                                            if (frame) {
                                                bool ok = true;
                                                int sampleRate = cc->sample_rate;
                                                int sampleCount = 0;
                                                std::vector<unsigned char> sampleBuffer;
                                                AVPacket pkt = { };
                                                av_init_packet(&pkt);
                                                while (av_read_frame(fc, &pkt) == 0) {
                                                    if (pkt.stream_index == streamId) {
                                                        if (avcodec_send_packet(cc, &pkt) < 0) {
                                                            av_packet_unref(&pkt);
                                                            ok = false;
                                                            break;
                                                        }
                                                        while (!avcodec_receive_frame(cc, frame)) {
                                                            storeSamples(sc, frame, sampleBuffer, sampleCount);
                                                        }
                                                    }
                                                    av_packet_unref(&pkt);
                                                }
                                                if (ok) {
                                                    if (avcodec_send_packet(cc, NULL) >= 0) {
                                                        while (!avcodec_receive_frame(cc, frame)) {
                                                            storeSamples(sc, frame, sampleBuffer, sampleCount);
                                                        }
                                                        output = new SoundDecoder(sampleRate, sampleCount, (std::vector<unsigned char> &&) sampleBuffer);
                                                    }
                                                }
                                                av_frame_free(&frame);
                                            }
                                        }
                                        swr_free(&sc);
                                    }
                                    avcodec_close(cc);
                                }
                            }
                            avcodec_free_context(&cc);
                        }
                    }
                }
                avformat_close_input(&fc);
            } else
                av_free(ioc);
        } else
            av_free(buffer);
        if (fc)
            avformat_free_context(fc);
    }
    return output;
}

SoundDecoder::SoundDecoder(int sampleRate, int sampleCount, std::vector<unsigned char> &&sampleBuffer) : sampleRate(sampleRate), sampleCount(sampleCount), sampleBuffer((std::vector<unsigned char> &&) sampleBuffer) { }

SoundDecoder::~SoundDecoder() { }

int SoundDecoder::getSampleRate() const {
    return sampleRate;
}

int SoundDecoder::getSampleCount() const {
    return sampleCount;
}

void SoundDecoder::copyWaveform(void *output, int samples) const {
    if (samples <= 0)
        return;
    size_t dataSize = SAMPLE_SIZE*samples;
    if (dataSize > sampleBuffer.size()) {
        if (sampleBuffer.size() > 0)
            memcpy(output, &sampleBuffer[0], sampleBuffer.size());
        memset(reinterpret_cast<unsigned char *>(output)+sampleBuffer.size(), 0, dataSize-sampleBuffer.size());
    } else
        memcpy(output, &sampleBuffer[0], dataSize);
}
