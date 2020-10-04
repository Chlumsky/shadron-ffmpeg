
#pragma once

#include <string>
#include "LogicalObject.h"

/// MP4 file export
class Mp4ExportObject : public LogicalObject {

public:
    enum Codec {
        H264,
        HEVC
    };

    enum PixelFormat {
        YUV420,
        YUV444,
    };

    Mp4ExportObject(int sourceId, const std::string &filename, Codec codec, PixelFormat pixelFormat, const std::string &settings, int framerateExpr, int durationExpr, float framerate, float duration, const LogicalObject *framerateSource, const LogicalObject *durationSource);
    Mp4ExportObject(const Mp4ExportObject &) = delete;
    virtual ~Mp4ExportObject();
    Mp4ExportObject & operator=(const Mp4ExportObject &) = delete;
    Mp4ExportObject * reconfigure(int sourceId, const std::string &filename, Codec codec, PixelFormat pixelFormat, const std::string &settings, int framerateExpr, int durationExpr, float framerate, float duration, const LogicalObject *framerateSource, const LogicalObject *durationSource);
    virtual bool setExpressionValue(int exprId, ExpressionType type, const void *value) override;
    virtual bool offerSource(int sourceId) const override;
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height) override;
    virtual bool startExport() override;
    virtual void finishExport() override;
    virtual int getExportStepCount() const override;
    virtual std::string getExportFilename() const override;
    virtual bool prepareExportStep(int step, float &time, float &deltaTime) override;
    virtual bool exportStep() override;

private:
    struct Mp4ExportData;

    Mp4ExportData *data;
    int sourceId;
    std::string filename;
    Codec codec;
    PixelFormat pixelFormat;
    std::string settings;
    int framerateExpr;
    int durationExpr;
    float framerate;
    float duration;
    const LogicalObject *framerateSource;
    const LogicalObject *durationSource;
    int frameCount;
    float frameDuration;
    int step;
    int width, height;

};
