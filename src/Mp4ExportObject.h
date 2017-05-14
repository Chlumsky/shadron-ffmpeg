
#pragma once

#include <string>
#include "LogicalObject.h"

/// MP4 file export
class Mp4ExportObject : public LogicalObject {
    friend struct Mp4ExportData;

public:
    enum Codec {
        H264,
        HEVC
    };

    enum PixelFormat {
        YUV420,
        YUV444,
    };

    Mp4ExportObject(int sourceId, const std::string &filename, Codec codec, PixelFormat pixelFormat, const std::string &settings, float framerate, float duration);
    Mp4ExportObject(const Mp4ExportObject &) = delete;
    virtual ~Mp4ExportObject();
    Mp4ExportObject & operator=(const Mp4ExportObject &) = delete;
    virtual bool offerSource(int sourceId) const;
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height);
    virtual bool startExport();
    virtual void finishExport();
    virtual int getExportStepCount() const;
    virtual std::string getExportFilename() const;
    virtual bool prepareExportStep(int step, float &time, float &deltaTime);
    virtual bool exportStep();

private:
    Mp4ExportData *data;
    int sourceId;
    std::string filename;
    Codec codec;
    PixelFormat pixelFormat;
    std::string settings;
    float framerate;
    float duration;
    int frameCount;
    float frameDuration;
    int step;
    int width, height;

};
