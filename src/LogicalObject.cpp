
#include "LogicalObject.h"

LogicalObject::LogicalObject(const std::string &name) : name(name) { }

const std::string & LogicalObject::getName() const {
    return name;
}

bool LogicalObject::prepare(int &width, int &height, bool hardReset, bool repeat) {
    return false;
}

bool LogicalObject::getSize(int &width, int &height) const {
    return false;
}

bool LogicalObject::getFramerate(int &num, int &den) const {
    return false;
}

bool LogicalObject::getDuration(float &duration) const {
    return false;
}

bool LogicalObject::acceptsFiles() const {
    return false;
}

bool LogicalObject::loadFile(const char *filename) {
    return false;
}

void LogicalObject::unloadFile() { }

bool LogicalObject::restart() {
    return false;
}

bool LogicalObject::offerSource(int sourceId) const {
    return false;
}

void LogicalObject::setSourcePixels(int sourceId, const void *pixels, int width, int height) { }

bool LogicalObject::pixelsReady() const {
    return false;
}

const void * LogicalObject::fetchPixels(float time, float deltaTime, bool realTime, int width, int height) {
    return NULL;
}

bool LogicalObject::startExport() {
    return false;
}

void LogicalObject::finishExport() { }

int LogicalObject::getExportStepCount() const {
    return 0;
}

std::string LogicalObject::getExportFilename() const {
    return std::string();
}

bool LogicalObject::prepareExportStep(int step, float &time, float &deltaTime) {
    return false;
}

bool LogicalObject::exportStep() {
    return false;
}
