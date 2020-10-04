
#pragma once

/*
 * SHADRON FFMPEG EXTENSION by Viktor Chlumsky (c) 2017
 * 
 * This extension allows the user to load video files as input animations,
 * export animations as video files, and load additional audio file formats,
 * via the FFmpeg library.
 * 
 * http://www.shadron.info
 */

#include <cstdlib>
#include <string>
#include <map>
#include "LogicalObject.h"

#define EXTENSION_NAME "ffmpeg"
#define EXTENSION_VERSION 130

#define INITIALIZER_VIDEO_FILE_ID 0
#define INITIALIZER_VIDEO_FILE_NAME "video_file"

#define INITIALIZER_MP4_EXPORT_ID 1
#define INITIALIZER_MP4_EXPORT_NAME "mp4"

#define ERROR_EXPORT_SOURCE_TYPE "Only animation objects may be exported as video files"
#define ERROR_FORMAT_KEYWORD "The supported video compression formats are h264 and hevc"
#define ERROR_COLOR_KEYWORD "Color format (yuv420 or yuv444), encoder settings or video framerate expected"
#define ERROR_FRAMERATE_POSITIVE "The video frame rate must be a positive floating point value"
#define ERROR_DURATION_NONNEGATIVE "The video duration must be a positive time in seconds"
#define ERROR_DEPENDENCY_NOT_FOUND " does not name a video_file object. If it is a value, please add + at the beginning"

class FfmpegExtension {

public:
    FfmpegExtension();
    FfmpegExtension(const FfmpegExtension &) = delete;
    ~FfmpegExtension();
    FfmpegExtension & operator=(const FfmpegExtension &) = delete;
    void refObject(LogicalObject *object);
    void unrefObject(LogicalObject *object);
    LogicalObject * findObject(const std::string &name) const;

private:
    std::map<LogicalObject *, int> objectRefs;
    std::map<std::string, LogicalObject *> objectLookup;

};
