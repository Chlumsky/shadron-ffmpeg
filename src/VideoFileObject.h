
#pragma once

#include <string>
#include "LogicalObject.h"

/// Video file animation object
class VideoFileObject : public LogicalObject {
    friend struct VideoFileData;

public:
    VideoFileObject(const std::string &name, const std::string &filename = std::string());
    VideoFileObject(const VideoFileObject &) = delete;
    virtual ~VideoFileObject();
    VideoFileObject & operator=(const VideoFileObject &) = delete;
    VideoFileObject * reconfigure(const std::string &filename = std::string());
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat) override;
    virtual bool getSize(int &width, int &height) const override;
    virtual bool getFramerate(int &num, int &den) const override;
    virtual bool getDuration(float &duration) const override;
    virtual bool acceptsFiles() const override;
    virtual bool loadFile(const char *filename) override;
    virtual void unloadFile() override;
    virtual bool restart() override;
    virtual bool pixelsReady() const override;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height) override;

private:
    VideoFileData *data;
    bool prepared;
    bool fileOpen;
    int width, height;
    bool repeat;
    std::string initialFilename;

    bool atStart, atLastFrame;
    long long frameEndTime;
    double frameRemainingTime;

    bool rewind();
    bool nextFrame();
    bool isFrameCurrent(float time, bool realTime);

};
