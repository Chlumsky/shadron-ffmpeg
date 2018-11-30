
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
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat);
    virtual bool getSize(int &width, int &height) const;
    virtual bool getFramerate(int &num, int &den) const;
    virtual bool getDuration(float &duration) const;
    virtual bool acceptsFiles() const;
    virtual bool loadFile(const char *filename);
    virtual void unloadFile();
    virtual bool restart();
    virtual bool pixelsReady() const;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height);

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
