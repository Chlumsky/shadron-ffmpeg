
# Shadron-FFmpeg extension

This is an official extension for [Shadron](http://www.shadron.info/)
which adds the functionality to load and export multimedia files via
the [FFmpeg library](https://ffmpeg.org/).

### List of features
 - `video_file` animation object, which loads video files
 - MP4 video file export
 - decoding additional audio file formats, including MP3

## Installation

Place `shadron-ffmpeg.dll` and the `shadron-ffmpeg` folder containing FFmpeg DLL files
in the `extensions` directory next to Shadron's executable, or better yet,
in `%APPDATA%\Shadron\extensions`. It will be automatically detected by Shadron on next launch.
Requires Shadron 1.1.3 or later.

## Usage

As soon as the extension is installed, you will be able to load additional
audio format into sound objects.
To load or export video files, you must first enable the extension with the directive:

    #extension ffmpeg

Create an input video file animation with:

    animation InputVideo = video_file("filename.mp4");

(The file name specification is optional just like in the `file` initializer.)

To export an animation as a video file, you may declare an MP4 export like this:

    export mp4(MyAnimation, "output.mp4", <codec>, <pixel format>, <encoder settings>, <framerate>, <duration>);

Currently, codec may be either `h264` or `hevc`.
The pixel format parameter is optional, and may be either `yuv420` (default) or `yuv444`.
The encoder settings is an optional string parameter that may contain
a sequence of key-value pairs (`key=value`), separated by commas.
For example, `preset=slow` lets the encoder take longer to better compress the video,
and `crf=16` sets the video quality (lower CRF = higher quality).
Refer to the [FFmpeg documentation](https://trac.ffmpeg.org/wiki/Encode/H.264)
for a list of possible values. The framerate (frames per second) and duration (seconds)
are the same as in `png_sequence` and other export types.
Framerate and duration may also be specified as the name of a previously declared `video_file`
animation and the properties of the loaded video file will be used.
Please note that for `yuv420`, both the width and height of the exported animation must be even,
otherwise the export will fail.
