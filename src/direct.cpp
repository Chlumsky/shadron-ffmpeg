
#include <cstdlib>
#include <shadron-api.h>
#include "Mp4ExportObject.h"

extern "C" {

SHADRON_API_FN void * direct_shadron_ffmpeg_export_initialize(const char *filename, const char *codec, const char *pixelFormat, const char *settings, int width, int height, float framerate) {
    Mp4ExportObject::Codec cod;
    Mp4ExportObject::PixelFormat fmt;
    std::string kw = reinterpret_cast<const char *>(codec);
    if (kw == "h264" || kw == "H264")
        cod = Mp4ExportObject::H264;
    else if (kw == "hevc" || kw == "HEVC" || kw == "h265" || kw == "H265")
        cod = Mp4ExportObject::HEVC;
    else if (kw == "av1" || kw == "AV1")
        cod = Mp4ExportObject::AV1;
    else
        return NULL;
    kw = reinterpret_cast<const char *>(pixelFormat);
    if (kw == "grayscale" || kw == "gray" || kw == "bw")
        fmt = Mp4ExportObject::GRAYSCALE;
    else if (kw == "yuv420" || kw == "YUV420")
        fmt = Mp4ExportObject::YUV420;
    else if (kw == "yuv444" || kw == "YUV444")
        fmt = Mp4ExportObject::YUV444;
    else if (kw == "yuva444" || kw == "YUVA444")
        fmt = Mp4ExportObject::YUVA444;
    else
        return NULL;
    Mp4ExportObject *obj = new Mp4ExportObject(0, filename, cod, fmt, settings, -1, -1, framerate, 0.f, NULL, NULL);
    if (!obj->startExport()) {
        delete obj;
        return NULL;
    }
    return obj;
}

SHADRON_API_FN int direct_shadron_ffmpeg_export_step(void *context, const void *pixels, int width, int height) {
    Mp4ExportObject *obj = reinterpret_cast<Mp4ExportObject *>(context);
    float dummyTime = 0.f, dummyDeltaTime = 0.f;
    if (!obj->prepareExportStep(obj->getLastStepIndex()+1, dummyTime, dummyDeltaTime)) {
        delete obj;
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    obj->setSourcePixels(0, pixels, width, height);
    if (!obj->exportStep()) {
        delete obj;
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    return SHADRON_RESULT_OK;
}

SHADRON_API_FN int direct_shadron_ffmpeg_export_end(void *context) {
    Mp4ExportObject *obj = reinterpret_cast<Mp4ExportObject *>(context);
    if (!obj->finalStep()) {
        delete obj;
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    obj->finishExport();
    delete obj;
    return SHADRON_RESULT_OK;
}

}
