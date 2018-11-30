
#include <cstdlib>
#include <cstring>
#include <string>
#include <shadron-api.h>
#include "FfmpegExtension.h"
#include "LogicalObject.h"
#include "VideoFileObject.h"
#include "Mp4ExportObject.h"
#include "SoundDecoder.h"

struct ParseData {
    int initializer;
    int curArg;
    std::string filename;
    int sourceId;
    Mp4ExportObject::Codec codec;
    Mp4ExportObject::PixelFormat pixelFormat;
    std::string settings;
    float framerate;
    float duration;
    std::string framerateSourceName;
    std::string durationSourceName;
    const LogicalObject *framerateSource;
    const LogicalObject *durationSource;
};

extern "C" {

int __declspec(dllexport) shadron_register_extension(int *magicNumber, int *flags, char *name, int *nameLength, int *version, void **context) {
    *magicNumber = SHADRON_MAGICNO;
    *flags = SHADRON_FLAG_ANIMATION|SHADRON_FLAG_EXPORT|SHADRON_FLAG_SOUND_DECODER|SHADRON_FLAG_CHARSET_UTF8;
    if (*nameLength <= sizeof(EXTENSION_NAME))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    memcpy(name, EXTENSION_NAME, sizeof(EXTENSION_NAME));
    *nameLength = sizeof(EXTENSION_NAME)-1;
    *version = EXTENSION_VERSION;
    *context = new FfmpegExtension;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_unregister_extension(void *context) {
    FfmpegExtension *ext = reinterpret_cast<FfmpegExtension *>(context);
    delete ext;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_register_initializer(void *context, int index, int *flags, char *name, int *nameLength) {
    switch (index) {
        case INITIALIZER_VIDEO_FILE_ID:
            if (*nameLength <= sizeof(INITIALIZER_VIDEO_FILE_NAME))
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            memcpy(name, INITIALIZER_VIDEO_FILE_NAME, sizeof(INITIALIZER_VIDEO_FILE_NAME));
            *nameLength = sizeof(INITIALIZER_VIDEO_FILE_NAME)-1;
            *flags = SHADRON_FLAG_ANIMATION;
            return SHADRON_RESULT_OK;
        case INITIALIZER_MP4_EXPORT_ID:
            if (*nameLength <= sizeof(INITIALIZER_MP4_EXPORT_NAME))
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            memcpy(name, INITIALIZER_MP4_EXPORT_NAME, sizeof(INITIALIZER_MP4_EXPORT_NAME));
            *nameLength = sizeof(INITIALIZER_MP4_EXPORT_NAME)-1;
            *flags = SHADRON_FLAG_EXPORT;
            return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_NO_MORE_ITEMS;
    }
}

int __declspec(dllexport) shadron_parse_initializer(void *context, int objectType, int index, const char *name, int nameLength, void **parseContext, int *firstArgumentTypes) {
    switch (index) {
        case INITIALIZER_VIDEO_FILE_ID:
            if (objectType != SHADRON_FLAG_ANIMATION)
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            *parseContext = new ParseData { INITIALIZER_VIDEO_FILE_ID };
            *firstArgumentTypes = SHADRON_ARG_NONE|SHADRON_ARG_FILENAME;
            return SHADRON_RESULT_OK;
        case INITIALIZER_MP4_EXPORT_ID:
            if (objectType != SHADRON_FLAG_EXPORT)
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            *parseContext = new ParseData { INITIALIZER_MP4_EXPORT_ID };
            *firstArgumentTypes = SHADRON_ARG_SOURCE_OBJ;
            return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
}

int __declspec(dllexport) shadron_parse_initializer_argument(void *context, void *parseContext, int argNo, int argumentType, const void *argumentData, int *nextArgumentTypes) {
    FfmpegExtension *ext = reinterpret_cast<FfmpegExtension *>(context);
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    switch (pd->initializer) {
        case INITIALIZER_VIDEO_FILE_ID:
            switch (pd->curArg) {
                case 0: // Input filename
                    if (argumentType != SHADRON_ARG_FILENAME)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->filename = reinterpret_cast<const char *>(argumentData);
                    *nextArgumentTypes = SHADRON_ARG_NONE;
                    break;
                default:
                    return SHADRON_RESULT_UNEXPECTED_ERROR;
            }
            break;
        case INITIALIZER_MP4_EXPORT_ID:
            switch (pd->curArg) {
                case 0: // Source animation name
                    if (argumentType != SHADRON_ARG_SOURCE_OBJ)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    if (reinterpret_cast<const int *>(argumentData)[1] != SHADRON_FLAG_ANIMATION)
                        return SHADRON_RESULT_PARSE_ERROR;
                    pd->sourceId = reinterpret_cast<const int *>(argumentData)[0];
                    *nextArgumentTypes = SHADRON_ARG_FILENAME;
                    break;
                case 1: // Output filename
                    if (argumentType != SHADRON_ARG_FILENAME)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->filename = reinterpret_cast<const char *>(argumentData);
                    *nextArgumentTypes = SHADRON_ARG_KEYWORD;
                    break;
                case 2: // Video compression format
                    if (argumentType != SHADRON_ARG_KEYWORD)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    {
                        std::string kw = reinterpret_cast<const char *>(argumentData);
                        if (kw == "h264" || kw == "H264")
                            pd->codec = Mp4ExportObject::H264;
                        else if (kw == "hevc" || kw == "HEVC" || kw == "h265" || kw == "H265")
                            pd->codec = Mp4ExportObject::HEVC;
                        else
                            return SHADRON_RESULT_PARSE_ERROR;
                    }
                    *nextArgumentTypes = SHADRON_ARG_KEYWORD|SHADRON_ARG_STRING|SHADRON_ARG_FLOAT;
                    break;
                case 3: // Video pixel format (optional)
                    if (argumentType == SHADRON_ARG_KEYWORD) {
                        std::string kw = reinterpret_cast<const char *>(argumentData);
                        if (kw == "yuv420" || kw == "YUV420")
                            pd->pixelFormat = Mp4ExportObject::YUV420;
                        else if (kw == "yuv444" || kw == "YUV444")
                            pd->pixelFormat = Mp4ExportObject::YUV444;
                        else
                            return SHADRON_RESULT_PARSE_ERROR;
                        *nextArgumentTypes = SHADRON_ARG_STRING|SHADRON_ARG_FLOAT|SHADRON_ARG_KEYWORD;
                        break;
                    } else
                        pd->pixelFormat = Mp4ExportObject::YUV420;
                    ++pd->curArg;
                case 4: // Settings (optional)
                    if (argumentType == SHADRON_ARG_STRING) {
                        pd->settings = reinterpret_cast<const char *>(argumentData);
                        *nextArgumentTypes = SHADRON_ARG_FLOAT|SHADRON_ARG_KEYWORD;
                        break;
                    }
                    ++pd->curArg;
                case 5: // Video framerate
                    if (argumentType == SHADRON_ARG_FLOAT) {
                        pd->framerate = *reinterpret_cast<const float *>(argumentData);
                        if (pd->framerate <= 0.f)
                            return SHADRON_RESULT_PARSE_ERROR;
                        *nextArgumentTypes = SHADRON_ARG_FLOAT|SHADRON_ARG_KEYWORD;
                    } else if (argumentType == SHADRON_ARG_KEYWORD) {
                        pd->framerateSourceName = reinterpret_cast<const char *>(argumentData);
                        pd->framerateSource = ext->findObject(pd->framerateSourceName);
                        if (!pd->framerateSource)
                            return SHADRON_RESULT_PARSE_ERROR;
                        pd->durationSource = pd->framerateSource;
                        *nextArgumentTypes = SHADRON_ARG_NONE|SHADRON_ARG_FLOAT|SHADRON_ARG_KEYWORD;
                    } else
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    break;
                case 6: // Video duration
                    pd->durationSource = NULL;
                    if (argumentType == SHADRON_ARG_FLOAT) {
                        pd->duration = *reinterpret_cast<const float *>(argumentData);
                        if (pd->duration <= 0.f)
                            return SHADRON_RESULT_PARSE_ERROR;
                    } else if (argumentType == SHADRON_ARG_KEYWORD) {
                        pd->durationSourceName = reinterpret_cast<const char *>(argumentData);
                        pd->durationSource = ext->findObject(pd->durationSourceName);
                        if (!pd->durationSource)
                            return SHADRON_RESULT_PARSE_ERROR;
                    } else
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    *nextArgumentTypes = SHADRON_ARG_NONE;
                    break;
                default:
                    return SHADRON_RESULT_UNEXPECTED_ERROR;
            }
            break;
        default:
            return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    ++pd->curArg;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_parse_initializer_finish(void *context, void *parseContext, int result, int objectType, const char *objectName, int nameLength, void **object) {
    FfmpegExtension *ext = reinterpret_cast<FfmpegExtension *>(context);
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    int newResult = SHADRON_RESULT_OK;
    if (result == SHADRON_RESULT_OK) {
        std::string name(objectName);
        LogicalObject *obj = ext->findObject(name);
        if (obj) {
            switch (pd->initializer) {
                case INITIALIZER_VIDEO_FILE_ID:
                    obj = dynamic_cast<VideoFileObject *>(obj);
                    break;
                case INITIALIZER_MP4_EXPORT_ID:
                    obj = dynamic_cast<Mp4ExportObject *>(obj);
                    break;
                default:
                    obj = NULL;
            }
        }
        if (!obj) {
            switch (pd->initializer) {
                case INITIALIZER_VIDEO_FILE_ID:
                    obj = new VideoFileObject(name, pd->filename);
                    break;
                case INITIALIZER_MP4_EXPORT_ID:
                    obj = new Mp4ExportObject(pd->sourceId, pd->filename, pd->codec, pd->pixelFormat, pd->settings, pd->framerate, pd->duration, pd->framerateSource, pd->durationSource);
                    break;
                default:
                    newResult = SHADRON_RESULT_UNEXPECTED_ERROR;
            }
        }
        ext->refObject(obj);
        *object = obj;
    }
    delete pd;
    return newResult;
}

int __declspec(dllexport) shadron_parse_error_length(void *context, void *parseContext, int *length, int encoding) {
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    switch (pd->initializer) {
        case INITIALIZER_MP4_EXPORT_ID:
            switch (pd->curArg) {
                case 0: // Source animation name
                    *length = sizeof(ERROR_EXPORT_SOURCE_TYPE)-1;
                    return SHADRON_RESULT_OK;
                case 2: // Video compression format
                    *length = sizeof(ERROR_FORMAT_KEYWORD)-1;
                    return SHADRON_RESULT_OK;
                case 3: // Video pixel format / encoder settings / framerate
                    *length = sizeof(ERROR_COLOR_KEYWORD)-1;
                    return SHADRON_RESULT_OK;
                case 5: // Video framerate
                    if (!pd->framerateSourceName.empty()) {
                        pd->framerateSourceName += ERROR_DEPENDENCY_NOT_FOUND;
                        *length = (int) pd->framerateSourceName.size();
                    } else
                        *length = sizeof(ERROR_FRAMERATE_POSITIVE)-1;
                    return SHADRON_RESULT_OK;
                case 6: // Video duration
                    if (!pd->durationSourceName.empty()) {
                        pd->durationSourceName += ERROR_DEPENDENCY_NOT_FOUND;
                        *length = (int) pd->durationSourceName.size();
                    } else
                        *length = sizeof(ERROR_DURATION_NONNEGATIVE)-1;
                    return SHADRON_RESULT_OK;
            }
        default:
            return SHADRON_RESULT_NO_DATA;
    }
}

int __declspec(dllexport) shadron_parse_error_string(void *context, void *parseContext, void *buffer, int *length, int bufferEncoding) {
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    const char *errorString = NULL;
    int errorStrLen = 0;
    switch (pd->initializer) {
        case INITIALIZER_MP4_EXPORT_ID:
            switch (pd->curArg) {
                case 0: // Source animation name
                    errorString = ERROR_EXPORT_SOURCE_TYPE;
                    errorStrLen = sizeof(ERROR_EXPORT_SOURCE_TYPE)-1;
                    break;
                case 2: // Video compression format
                    errorString = ERROR_FORMAT_KEYWORD;
                    errorStrLen = sizeof(ERROR_FORMAT_KEYWORD)-1;
                    break;
                case 3: // Video pixel format / encoder settings / framerate
                    errorString = ERROR_COLOR_KEYWORD;
                    errorStrLen = sizeof(ERROR_COLOR_KEYWORD)-1;
                    break;
                case 5: // Video framerate
                    if (!pd->framerateSourceName.empty()) {
                        errorString = pd->framerateSourceName.c_str();
                        errorStrLen = (int) pd->framerateSourceName.size();
                    } else {
                        errorString = ERROR_FRAMERATE_POSITIVE;
                        errorStrLen = sizeof(ERROR_FRAMERATE_POSITIVE)-1;
                    }
                    break;
                case 6: // Video duration
                    if (!pd->durationSourceName.empty()) {
                        errorString = pd->durationSourceName.c_str();
                        errorStrLen = (int) pd->durationSourceName.size();
                    } else {
                        errorString = ERROR_DURATION_NONNEGATIVE;
                        errorStrLen = sizeof(ERROR_DURATION_NONNEGATIVE)-1;
                    }
                    break;
            }
        default:;
    }
    if (errorString) {
        if (*length < errorStrLen || bufferEncoding != SHADRON_FLAG_CHARSET_UTF8)
            return SHADRON_RESULT_UNEXPECTED_ERROR;
        memcpy(buffer, errorString, errorStrLen);
        *length = errorStrLen;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_NO_DATA;
}

int __declspec(dllexport) shadron_object_prepare(void *context, void *object, int *flags, int *width, int *height, int *format) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->prepare(*width, *height, (*flags&SHADRON_FLAG_HARD_RESET) != 0, (*flags&SHADRON_FLAG_REPEAT) != 0))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    *flags &= SHADRON_FLAG_REPEAT;
    if (obj->acceptsFiles())
        *flags |= SHADRON_FLAG_FILE_INPUT;
    *format = SHADRON_FORMAT_RGBA_BYTE;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_size(void *context, void *object, int *width, int *height, int *format) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->getSize(*width, *height))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    *format = SHADRON_FORMAT_RGBA_BYTE;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_load_file(void *context, void *object, const void *path, int pathLength, int pathEncoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->loadFile(reinterpret_cast<const char *>(path)))
        return SHADRON_RESULT_FILE_IO_ERROR;
    return SHADRON_RESULT_OK_RESIZE;
}

int __declspec(dllexport) shadron_object_unload_file(void *context, void *object) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    obj->unloadFile();
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_set_expression_value(void *context, void *object, int exprIndex, int valueType, const void *value) {
    return SHADRON_RESULT_IGNORE;
}

int __declspec(dllexport) shadron_object_offer_source_pixels(void *context, void *object, int sourceIndex, int sourceType, int width, int height, int *format, void **pixelBuffer, void **pixelsContext) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (obj->offerSource(sourceIndex)) {
        *format = SHADRON_FORMAT_RGBA_BYTE;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_IGNORE;
}

int __declspec(dllexport) shadron_object_post_source_pixels(void *context, void *object, void *pixelsContext, int sourceIndex, int plane, int width, int height, int format, const void *pixels) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (format != SHADRON_FORMAT_RGBA_BYTE)
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    obj->setSourcePixels(sourceIndex, pixels, width, height);
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_user_command(void *context, void *object, int command) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    switch (command) {
        case SHADRON_COMMAND_RESTART:
            if (obj->restart())
                return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_NO_CHANGE;
    }
}

int __declspec(dllexport) shadron_object_destroy(void *context, void *object) {
    FfmpegExtension *ext = reinterpret_cast<FfmpegExtension *>(context);
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    ext->unrefObject(obj);
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_fetch_pixels(void *context, void *object, float time, float deltaTime, int realTime, int plane, int width, int height, int format, const void **pixels, void **pixelsContext) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->pixelsReady())
        return SHADRON_RESULT_NO_DATA;
    if (!(*pixels = obj->fetchPixels(time, deltaTime, realTime != 0, width, height)))
        return SHADRON_RESULT_NO_CHANGE;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_release_pixels(void *context, void *object, void *pixelsContext) {
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_object_start_export(void *context, void *object, int *stepCount, void **exportData) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->startExport())
        return SHADRON_RESULT_NO_DATA;
    *stepCount = obj->getExportStepCount();
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_export_prepare_step(void *context, void *object, void *exportData, int step, float *time, float *deltaTime, int *outputFilenameLength, int filenameEncoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    const std::string *outputFilename = NULL;
    if (!obj->prepareExportStep(step, *time, *deltaTime))
        return SHADRON_RESULT_NO_MORE_ITEMS;
    if (step == 0)
        *outputFilenameLength = (int) obj->getExportFilename().size();
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_export_output_filename(void *context, void *object, void *exportData, int step, void *buffer, int *length, int encoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    std::string filename = obj->getExportFilename();
    if (encoding != SHADRON_FLAG_CHARSET_UTF8 || *length < (int) filename.size())
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    if (filename.size() == 0)
        return SHADRON_RESULT_NO_DATA;
    memcpy(buffer, &filename[0], filename.size());
    *length = (int) filename.size();
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_export_step(void *context, void *object, void *exportData, int step, float time, float deltaTime) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->exportStep())
        return SHADRON_RESULT_FILE_IO_ERROR;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_export_finish(void *context, void *object, void *exportData, int result) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    obj->finishExport();
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_decode_sound(void *context, const void *rawData, int rawLength, int *sampleRate, int *sampleCount, int *format, void **decoderContext) {
    if (*format != SHADRON_FORMAT_STEREO_INT16LE)
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    SoundDecoder *decoder = SoundDecoder::decode(rawData, rawLength);
    if (decoder) {
        *sampleRate = decoder->getSampleRate();
        *sampleCount = decoder->getSampleCount();
        *decoderContext = decoder;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_FILE_FORMAT_ERROR;
}

int __declspec(dllexport) shadron_decode_fetch_samples(void *context, void *decoderContext, const void *rawData, int rawLength, void *sampleBuffer, int sampleCount, int format) {
    SoundDecoder *decoder = reinterpret_cast<SoundDecoder *>(decoderContext);
    if (format != SHADRON_FORMAT_STEREO_INT16LE) {
        delete decoder;
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    decoder->copyWaveform(sampleBuffer, sampleCount);
    delete decoder;
    return SHADRON_RESULT_OK;
}

int __declspec(dllexport) shadron_decode_discard(void *context, void *decoderContext) {
    SoundDecoder *decoder = reinterpret_cast<SoundDecoder *>(decoderContext);
    delete decoder;
    return SHADRON_RESULT_OK;
}

}
