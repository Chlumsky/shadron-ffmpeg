
#include "FfmpegExtension.h"

extern "C" {
    #include <libavformat/avformat.h>
}

FfmpegExtension::FfmpegExtension() {
    av_register_all();
}

FfmpegExtension::~FfmpegExtension() {
    for (std::map<LogicalObject *, int>::iterator entry = objectRefs.begin(); entry != objectRefs.end(); ++entry)
        if (entry->second > 0)
            delete entry->first;
}

void FfmpegExtension::refObject(LogicalObject *object) {
    if (!object)
        return;
    int &refCount = objectRefs[object];
    if (refCount == 0)
        objectLookup[object->getName()] = object;
    ++refCount;
}

void FfmpegExtension::unrefObject(LogicalObject *object) {
    if (!object)
        return;
    int &refCount = objectRefs[object];
    --refCount;
    if (refCount == 0) {
        std::map<std::string, LogicalObject *>::iterator entry = objectLookup.find(object->getName());
        if (entry != objectLookup.end() && entry->second == object)
            objectLookup.erase(entry);
        delete object;
    }
}

LogicalObject * FfmpegExtension::findObject(const std::string &name) const {
    if (!name.empty()) {
        std::map<std::string, LogicalObject *>::const_iterator entry = objectLookup.find(name);
        if (entry != objectLookup.end())
            return entry->second;
    }
    return NULL;
}
