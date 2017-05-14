
/* Shadron 1.1.3 extension API */

#ifndef SHADRON_API_H
#define SHADRON_API_H

#define SHADRON_MAGICNO 0x76e9d70d

#define SHADRON_RESULT_OK                 0
#define SHADRON_RESULT_NO_DATA            1
#define SHADRON_RESULT_NO_MORE_ITEMS      2
#define SHADRON_RESULT_NO_CHANGE          3
#define SHADRON_RESULT_CANCELLED          4
#define SHADRON_RESULT_IGNORE             5
#define SHADRON_RESULT_OK_RESIZE          6
#define SHADRON_RESULT_PARSE_ERROR        8
#define SHADRON_RESULT_FILE_IO_ERROR     16
#define SHADRON_RESULT_FILE_FORMAT_ERROR 17
#define SHADRON_RESULT_UNEXPECTED_ERROR (-1)

#define SHADRON_FLAG_IMAGE             0x0001
#define SHADRON_FLAG_ANIMATION         0x0002
#define SHADRON_FLAG_EXPORT            0x0010
#define SHADRON_FLAG_STREAM            0x0020
#define SHADRON_FLAG_IMAGE_DECODER     0x0040
#define SHADRON_FLAG_SOUND_DECODER     0x0080

#define SHADRON_FLAG_FULL_RANGE        0x0100
#define SHADRON_FLAG_REPEAT            0x0200
#define SHADRON_FLAG_FILE_INPUT        0x0400
#define SHADRON_FLAG_HARD_RESET        0x0800

#define SHADRON_FLAG_CHARSET_UTF8      0x1000
#define SHADRON_FLAG_CHARSET_UTF16LE   0x2000

#define SHADRON_ARG_NONE               0x0001
#define SHADRON_ARG_KEYWORD            0x0002
#define SHADRON_ARG_FILENAME           0x0004
#define SHADRON_ARG_STRING             0x0008
#define SHADRON_ARG_INT                0x0010
#define SHADRON_ARG_FLOAT              0x0020
#define SHADRON_ARG_BOOL               0x0040
#define SHADRON_ARG_DIMENSIONS         0x0080
#define SHADRON_ARG_EXPR_INT           0x0100
#define SHADRON_ARG_EXPR_FLOAT         0x0200
#define SHADRON_ARG_EXPR_BOOL          0x0400
#define SHADRON_ARG_EXPR_DIMENSIONS    0x0800
#define SHADRON_ARG_SOURCE_OBJ         0x1000

#define SHADRON_FORMAT_RGBA_BYTE      0x0040
#define SHADRON_FORMAT_RGBA_FLOAT     0x0140
#define SHADRON_FORMAT_STEREO_INT16LE 0x5000

#define SHADRON_COMMAND_RESTART 1

#ifdef __cplusplus
extern "C" {
#endif

int __declspec(dllexport) shadron_register_extension(int *magicNumber, int *flags, char *name, int *nameLength, int *version, void **context);
int __declspec(dllexport) shadron_unregister_extension(void *context);

int __declspec(dllexport) shadron_register_initializer(void *context, int index, int *flags, char *name, int *nameLength);
int __declspec(dllexport) shadron_parse_initializer(void *context, int objectType, int index, const char *name, int nameLength, void **parseContext, int *firstArgumentTypes);
int __declspec(dllexport) shadron_parse_initializer_argument(void *context, void *parseContext, int argNo, int argumentType, const void *argumentData, int *nextArgumentTypes);
int __declspec(dllexport) shadron_parse_initializer_finish(void *context, void *parseContext, int result, int objectType, const char *objectName, int nameLength, void **object);
int __declspec(dllexport) shadron_parse_error_length(void *context, void *parseContext, int *length, int encoding);
int __declspec(dllexport) shadron_parse_error_string(void *context, void *parseContext, void *buffer, int *length, int bufferEncoding);

int __declspec(dllexport) shadron_object_prepare(void *context, void *object, int *flags, int *width, int *height, int *format);
int __declspec(dllexport) shadron_object_size(void *context, void *object, int *width, int *height, int *format);
int __declspec(dllexport) shadron_object_load_file(void *context, void *object, const void *path, int pathLength, int pathEncoding);
int __declspec(dllexport) shadron_object_unload_file(void *context, void *object);
int __declspec(dllexport) shadron_object_set_expression_value(void *context, void *object, int exprIndex, int valueType, const void *value);
int __declspec(dllexport) shadron_object_offer_source_pixels(void *context, void *object, int sourceIndex, int sourceType, int width, int height, int *format, void **pixelBuffer, void **pixelsContext);
int __declspec(dllexport) shadron_object_post_source_pixels(void *context, void *object, void *pixelsContext, int sourceIndex, int plane, int width, int height, int format, const void *pixels);
int __declspec(dllexport) shadron_object_user_command(void *context, void *object, int command);
int __declspec(dllexport) shadron_object_destroy(void *context, void *object);

int __declspec(dllexport) shadron_object_fetch_pixels(void *context, void *object, float time, float deltaTime, int realTime, int plane, int width, int height, int format, const void **pixels, void **pixelsContext);
int __declspec(dllexport) shadron_object_release_pixels(void *context, void *object, void *pixelsContext);

int __declspec(dllexport) shadron_object_start_export(void *context, void *object, int *stepCount, void **exportData);
int __declspec(dllexport) shadron_export_prepare_step(void *context, void *object, void *exportData, int step, float *time, float *deltaTime, int *outputFilenameLength, int filenameEncoding);
int __declspec(dllexport) shadron_export_output_filename(void *context, void *object, void *exportData, int step, void *buffer, int *length, int encoding);
int __declspec(dllexport) shadron_export_step(void *context, void *object, void *exportData, int step, float time, float deltaTime);
int __declspec(dllexport) shadron_export_finish(void *context, void *object, void *exportData, int result);

int __declspec(dllexport) shadron_object_start_stream(void *context, void *object, void **streamData);
int __declspec(dllexport) shadron_stream_prepare_frame(void *context, void *object, void *streamData, int step, float time, float deltaTime);
int __declspec(dllexport) shadron_stream_stop(void *context, void *object, void *streamData);

int __declspec(dllexport) shadron_decode_image(void *context, const void *rawData, int rawLength, int *width, int *height, int *format, void **decoderContext);
int __declspec(dllexport) shadron_decode_sound(void *context, const void *rawData, int rawLength, int *sampleRate, int *sampleCount, int *format, void **decoderContext);
int __declspec(dllexport) shadron_decode_fetch_pixels(void *context, void *decoderContext, const void *rawData, int rawLength, void *pixelBuffer, int width, int height, int format);
int __declspec(dllexport) shadron_decode_fetch_samples(void *context, void *decoderContext, const void *rawData, int rawLength, void *sampleBuffer, int sampleCount, int format);
int __declspec(dllexport) shadron_decode_discard(void *context, void *decoderContext);

#ifdef __cplusplus
}
#endif

#endif
