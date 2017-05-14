
#pragma once

#include <string>

/// Represents an abstract Shadron object (such as an animation or an export)
class LogicalObject {

public:
    virtual ~LogicalObject() { }
    const std::string & getName() const;
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat);
    virtual bool getSize(int &width, int &height) const;
    virtual bool acceptsFiles() const;
    virtual bool loadFile(const char *filename);
    virtual void unloadFile();
    virtual bool restart();
    virtual bool offerSource(int sourceId) const;
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height);
    virtual bool pixelsReady() const;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height);
    virtual bool startExport();
    virtual void finishExport();
    virtual int getExportStepCount() const;
    virtual std::string getExportFilename() const;
    virtual bool prepareExportStep(int step, float &time, float &deltaTime);
    virtual bool exportStep();

protected:
    LogicalObject(const std::string &name);

private:
    std::string name;

};
