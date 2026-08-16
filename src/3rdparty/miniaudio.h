// miniaudio.h - Single file audio playback library.
// Version 0.11.15
//
// This is a placeholder. To use real miniaudio:
// 1. Download the latest miniaudio.h from https://github.com/mackron/miniaudio/blob/master/miniaudio.h?raw=true
// 2. Replace this file with the downloaded one.
// 3. Ensure MINIAUDIO_IMPLEMENTATION is defined in exactly one source file (see AudioSystem.cpp).
//
// Copyright (c) 2018-2022 David Reid - mackron@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef miniaudio_h
#define miniaudio_h

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Options
// =============================================================================
// Define MA_NO_* to exclude certain backends.
//#define MA_NO_WASAPI
//#define MA_NO_DIRECTSOUND
//#define MA_NO_WINMM
//#define MA_NO_COREAUDIO
//#define MA_NO_ALSA
//#define MA_NO_PULSEAUDIO
//#define MA_NO_JACK
//#define MA_NO_SNDIO
//#define MA_NO_AUDIO4
//#define MA_NO_OSS
//#define MA_NO_OPENSL
//#define MA_NO_OPENAL
//#define MA_NO_SDL
//#define MA_NO_ANDROID
//#define MA_NO_DUMMY

// =============================================================================
// Types
// =============================================================================
typedef unsigned char       ma_bool8;
typedef ma_bool8            ma_bool32;
typedef signed char         ma_int8;
typedef unsigned char       ma_uint8;
typedef signed short        ma_int16;
typedef unsigned short      ma_uint16;
typedef signed int          ma_int32;
typedef unsigned int        ma_uint32;
typedef signed long long    ma_int64;
typedef unsigned long long  ma_uint64;
typedef float               ma_float32;
typedef double              ma_float64;

// Result codes.
typedef enum
{
    MA_SUCCESS                            =  0,
    MA_ERROR                              =  -1,  // Generic error.
    MA_INVALID_ARGS                       =  -2,
    MA_INVALID_OPERATION                  =  -3,
    MA_OUT_OF_MEMORY                      =  -4,
    MA_OUT_OF_RANGE                       =  -5,
    MA_ACCESS_DENIED                      =  -6,
    MA_DOES_NOT_EXIST                     =  -7,
    MA_ALREADY_EXISTS                     =  -8,
    MA_TOO_MANY_OPEN_FILES                =  -9,
    MA_INVALID_FILE                       =  -10,
    MA_TOO_BIG                            =  -11,
    MA_PATH_TOO_LONG                      =  -12,
    MA_NAME_TOO_LONG                      =  -13,
    MA_NOT_DIRECTORY                      =  -14,
    MA_IS_DIRECTORY                       =  -15,
    MA_DIRECTORY_NOT_EMPTY                =  -16,
    MA_END_OF_FILE                        =  -17,
    MA_NO_SPACE                           =  -18,
    MA_BUSY                               =  -19,
    MA_IO_ERROR                           =  -20,
    MA_INTERRUPT                          =  -21,
    MA_UNAVAILABLE                        =  -22,
    MA_ALREADY_LOCKED                     =  -23,
    MA_TIMEOUT                            =  -24,
    MA_NOT_IMPLEMENTED                    =  -25,
    MA_NO_DRIVER                          =  -26,
    MA_DEVICE_BUSY                        =  -27,
    MA_DEVICE_NOT_INITIALIZED             =  -28,
    MA_DEVICE_NOT_STARTED                 =  -29,
    MA_DEVICE_ALREADY_STARTED             =  -30,
    MA_DEVICE_NOT_STOPPED                 =  -31,
    MA_DEVICE_ALREADY_STOPPED             =  -32,
    MA_DEVICE_NOT_PAUSED                  =  -33,
    MA_DEVICE_ALREADY_PAUSED              =  -34,
    MA_DEVICE_LOOP_BROKEN                 =  -35,
    MA_INVALID_DEVICE_CONFIG              =  -36,
    MA_DEVICE_RESOURCES_ALLOC_FAILED      =  -37,
    MA_DEVICE_RESOURCES_ALLOC_FAILED_NO_MEM = -38,
    MA_DEVICE_RESOURCES_ALLOC_FAILED_NO_SYSMEM = -39,
    MA_DEVICE_RESOURCES_ALLOC_FAILED_NO_VIDMEM = -40,
    MA_DEVICE_RESOURCES_ALLOC_FAILED_NO_DRM = -41,
    MA_FORMAT_NOT_SUPPORTED               =  -42,
    MA_DEVICE_TYPE_NOT_SUPPORTED          =  -43,
    MA_DEVICE_SHARE_MODE_NOT_SUPPORTED    =  -44,
    MA_NO_DEVICE                          =  -45,
    MA_API_NOT_FOUND                      =  -46,
    MA_DEVICE_UNPLUGGED                   =  -47,
    MA_DEVICE_VIOLATED_SHARE_MODE         =  -48,
    MA_DATA_SOURCE_NOT_FOUND              =  -49,
    MA_DATA_SOURCE_BUSY                   =  -50,
    MA_DATA_SOURCE_CANCELLED              =  -51,
    MA_INVALID_DATA_SOURCE_CONFIG         =  -52,
    MA_DATA_SOURCE_NOT_STARTED            =  -53,
    MA_DATA_SOURCE_ALREADY_STARTED        =  -54,
    MA_DATA_SOURCE_NOT_STOPPED            =  -55,
    MA_DATA_SOURCE_ALREADY_STOPPED        =  -56,
    MA_DATA_SOURCE_NOT_PAUSED             =  -57,
    MA_DATA_SOURCE_ALREADY_PAUSED         =  -58,
    MA_DATA_SOURCE_LOOP_BROKEN            =  -59,
    MA_DATA_SOURCE_INVALID_OPERATION      =  -60,
    MA_DATA_SOURCE_INVALID_ARGS           =  -61,
    MA_DATA_SOURCE_OUT_OF_MEMORY          =  -62,
    MA_DATA_SOURCE_OUT_OF_RANGE           =  -63,
    MA_DATA_SOURCE_ACCESS_DENIED          =  -64,
    MA_DATA_SOURCE_DOES_NOT_EXIST         =  -65,
    MA_DATA_SOURCE_ALREADY_EXISTS         =  -66,
    MA_DATA_SOURCE_TOO_MANY_OPEN_FILES    =  -67,
    MA_DATA_SOURCE_INVALID_FILE           =  -68,
    MA_DATA_SOURCE_TOO_BIG                =  -69,
    MA_DATA_SOURCE_PATH_TOO_LONG          =  -70,
    MA_DATA_SOURCE_NAME_TOO_LONG          =  -71,
    MA_DATA_SOURCE_NOT_DIRECTORY          =  -72,
    MA_DATA_SOURCE_IS_DIRECTORY           =  -73,
    MA_DATA_SOURCE_DIRECTORY_NOT_EMPTY    =  -74,
    MA_DATA_SOURCE_END_OF_FILE            =  -75,
    MA_DATA_SOURCE_NO_SPACE               =  -76,
    MA_DATA_SOURCE_BUSY                   =  -77,
    MA_DATA_SOURCE_IO_ERROR               =  -78,
    MA_DATA_SOURCE_INTERRUPT              =  -79,
    MA_DATA_SOURCE_UNAVAILABLE            =  -80,
    MA_DATA_SOURCE_ALREADY_LOCKED         =  -81,
    MA_DATA_SOURCE_TIMEOUT                =  -82,
    MA_DATA_SOURCE_NOT_IMPLEMENTED        =  -83,
    MA_DATA_SOURCE_NO_DRIVER              =  -84,
    MA_DATA_SOURCE_DEVICE_BUSY            =  -85,
    MA_DATA_SOURCE_DEVICE_NOT_INITIALIZED =  -86,
    MA_DATA_SOURCE_DEVICE_NOT_STARTED     =  -87,
    MA_DATA_SOURCE_DEVICE_ALREADY_STARTED =  -88,
    MA_DATA_SOURCE_DEVICE_NOT_STOPPED     =  -89,
    MA_DATA_SOURCE_DEVICE_ALREADY_STOPPED =  -90,
    MA_DATA_SOURCE_DEVICE_NOT_PAUSED      =  -91,
    MA_DATA_SOURCE_DEVICE_ALREADY_PAUSED  =  -92,
    MA_DATA_SOURCE_DEVICE_LOOP_BROKEN     =  -93,
    MA_DATA_SOURCE_INVALID_DEVICE_CONFIG  =  -94,
    MA_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED = -95,
    MA_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_MEM = -96,
    MA_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_SYSMEM = -97,
    MA_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_VIDMEM = -98,
    MA_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_DRM = -99,
    MA_DATA_SOURCE_FORMAT_NOT_SUPPORTED   = -100,
    MA_DATA_SOURCE_DEVICE_TYPE_NOT_SUPPORTED = -101,
    MA_DATA_SOURCE_DEVICE_SHARE_MODE_NOT_SUPPORTED = -102,
    MA_DATA_SOURCE_NO_DEVICE              = -103,
    MA_DATA_SOURCE_API_NOT_FOUND          = -104,
    MA_DATA_SOURCE_DEVICE_UNPLUGGED       = -105,
    MA_DATA_SOURCE_DEVICE_VIOLATED_SHARE_MODE = -106,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_FOUND  = -107,
    MA_DATA_SOURCE_DATA_SOURCE_BUSY       = -108,
    MA_DATA_SOURCE_DATA_SOURCE_CANCELLED  = -109,
    MA_DATA_SOURCE_INVALID_DATA_SOURCE_CONFIG = -110,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_STARTED = -111,
    MA_DATA_SOURCE_DATA_SOURCE_ALREADY_STARTED = -112,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_STOPPED = -113,
    MA_DATA_SOURCE_DATA_SOURCE_ALREADY_STOPPED = -114,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_PAUSED = -115,
    MA_DATA_SOURCE_DATA_SOURCE_ALREADY_PAUSED = -116,
    MA_DATA_SOURCE_DATA_SOURCE_LOOP_BROKEN = -117,
    MA_DATA_SOURCE_DATA_SOURCE_INVALID_OPERATION = -118,
    MA_DATA_SOURCE_DATA_SOURCE_INVALID_ARGS = -119,
    MA_DATA_SOURCE_DATA_SOURCE_OUT_OF_MEMORY = -120,
    MA_DATA_SOURCE_DATA_SOURCE_OUT_OF_RANGE = -121,
    MA_DATA_SOURCE_DATA_SOURCE_ACCESS_DENIED = -122,
    MA_DATA_SOURCE_DATA_SOURCE_DOES_NOT_EXIST = -123,
    MA_DATA_SOURCE_DATA_SOURCE_ALREADY_EXISTS = -124,
    MA_DATA_SOURCE_DATA_SOURCE_TOO_MANY_OPEN_FILES = -125,
    MA_DATA_SOURCE_DATA_SOURCE_INVALID_FILE = -126,
    MA_DATA_SOURCE_DATA_SOURCE_TOO_BIG = -127,
    MA_DATA_SOURCE_DATA_SOURCE_PATH_TOO_LONG = -128,
    MA_DATA_SOURCE_DATA_SOURCE_NAME_TOO_LONG = -129,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_DIRECTORY = -130,
    MA_DATA_SOURCE_DATA_SOURCE_IS_DIRECTORY = -131,
    MA_DATA_SOURCE_DATA_SOURCE_DIRECTORY_NOT_EMPTY = -132,
    MA_DATA_SOURCE_DATA_SOURCE_END_OF_FILE = -133,
    MA_DATA_SOURCE_DATA_SOURCE_NO_SPACE = -134,
    MA_DATA_SOURCE_DATA_SOURCE_BUSY = -135,
    MA_DATA_SOURCE_DATA_SOURCE_IO_ERROR = -136,
    MA_DATA_SOURCE_DATA_SOURCE_INTERRUPT = -137,
    MA_DATA_SOURCE_DATA_SOURCE_UNAVAILABLE = -138,
    MA_DATA_SOURCE_DATA_SOURCE_ALREADY_LOCKED = -139,
    MA_DATA_SOURCE_DATA_SOURCE_TIMEOUT = -140,
    MA_DATA_SOURCE_DATA_SOURCE_NOT_IMPLEMENTED = -141,
    MA_DATA_SOURCE_DATA_SOURCE_NO_DRIVER = -142,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_BUSY = -143,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_NOT_INITIALIZED = -144,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_NOT_STARTED = -145,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_ALREADY_STARTED = -146,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_NOT_STOPPED = -147,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_ALREADY_STOPPED = -148,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_NOT_PAUSED = -149,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_ALREADY_PAUSED = -150,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_LOOP_BROKEN = -151,
    MA_DATA_SOURCE_DATA_SOURCE_INVALID_DEVICE_CONFIG = -152,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED = -153,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_MEM = -154,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_SYSMEM = -155,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_VIDMEM = -156,
    MA_DATA_SOURCE_DATA_SOURCE_DEVICE_RESOURCES_ALLOC_FAILED_NO_DRM = -157,
} ma_result;

// =============================================================================
// Data Format
// =============================================================================
typedef enum
{
    ma_format_unknown   = 0,
    ma_format_u8        = 1,
    ma_format_s16       = 2,
    ma_format_s24       = 3,
    ma_format_s32       = 4,
    ma_format_f32       = 5,
    ma_format_f64       = 6,
    ma_format_undefined = ma_format_unknown,
} ma_format;

// Channel count.
#define ma_max_channels     64

// Channel map.
typedef struct
{
    ma_uint8 channels[ma_max_channels];
    ma_uint8 channelCount;
} ma_channel_map;

// Channel position.
typedef enum
{
    ma_channel_none       =  0,
    ma_channel_mono       =  1,
    ma_channel_front_left =  2,
    ma_channel_front_right =  3,
    ma_channel_front_center =  4,
    ma_channel_lfe        =  5,
    ma_channel_back_left  =  6,
    ma_channel_back_right =  7,
    ma_channel_front_left_of_center  =  8,
    ma_channel_front_right_of_center =  9,
    ma_channel_back_center   =  10,
    ma_channel_side_left     =  11,
    ma_channel_side_right    =  12,
    ma_channel_top_center    =  13,
    ma_channel_top_front_left =  14,
    ma_channel_top_front_center =  15,
    ma_channel_top_front_right =  16,
    ma_channel_top_back_left =  17,
    ma_channel_top_back_center =  18,
    ma_channel_top_back_right =  19,
} ma_channel;

// =============================================================================
// Device
// =============================================================================
typedef struct ma_device ma_device;
typedef struct ma_device_config ma_device_config;

// Device type.
typedef enum
{
    ma_device_type_undefined = 0,
    ma_device_type_playback  = 1,
    ma_device_type_capture   = 2,
    ma_device_type_full_duplex = 3,
    ma_device_type_loopback  = 4,
} ma_device_type;

// Device share mode.
typedef enum
{
    ma_share_mode_shared  = 0,
    ma_share_mode_exclusive = 1,
} ma_share_mode;

// Device state.
typedef enum
{
    ma_device_state_uninitialized = 0,
    ma_device_state_stopped       = 1,
    ma_device_state_started       = 2,
    ma_device_state_paused        = 3,
} ma_device_state;

// Device info.
typedef struct
{
    ma_device_type    deviceType;
    ma_format         format;
    ma_uint32         channels;
    ma_uint32         sampleRate;
    ma_channel_map    channelMap;
    ma_uint32         periodSizeInFrames;
    ma_uint32         periodsInBuffer;
    ma_uint32         bufferSizeInFrames;
    ma_uint32         bufferSizeInMilliseconds;
    ma_uint32         sizeInBytes;
    ma_uint32         sizeInMilliseconds;
    ma_share_mode     shareMode;
} ma_device_info;

// Device description.
typedef struct
{
    const char* name;
    ma_device_info info;
} ma_device_desc;

// Device callback.
typedef void (* ma_device_callback)(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

// Device config.
struct ma_device_config
{
    ma_device_type    deviceType;
    const char*       pPlaybackDeviceID;  // Optional. Can be NULL.
    const char*       pCaptureDeviceID;   // Optional. Can be NULL.
    ma_share_mode     shareMode;
    ma_format         format;
    ma_uint32         channelCount;
    ma_uint32         sampleRate;
    ma_channel_map    channelMap;
    ma_uint32         bufferSizeInFrames;
    ma_uint32         bufferSizeInMilliseconds;
    ma_uint32         periodSizeInFrames;
    ma_uint32         periodsInBuffer;
    ma_device_callback  callback;
    void*             pUserData;
    ma_bool32         noClip;
    ma_bool32         noPreSilencedOutputBuffer;
    ma_bool32         usingDefaultFormat;
    ma_bool32         usingDefaultChannelCount;
    ma_bool32         usingDefaultSampleRate;
    ma_bool32         usingDefaultChannelMap;
};

// =============================================================================
// Data Source
// =============================================================================
typedef struct ma_data_source ma_data_source;

// Data source callback.
typedef ma_result (* ma_data_source_read_proc)(ma_data_source* pDataSource, void* pBufferOut, ma_uint64 framesToRead, ma_uint64* pFramesRead);
typedef ma_result (* ma_data_source_seek_proc)(ma_data_source* pDataSource, ma_uint64 frameIndex);
typedef ma_result (* ma_data_source_get_cursor_proc)(ma_data_source* pDataSource, ma_uint64* pCursor);
typedef ma_result (* ma_data_source_set_loop_point_proc)(ma_data_source* pDataSource, ma_uint64 frameIndex);
typedef ma_result (* ma_data_source_get_loop_point_proc)(ma_data_source* pDataSource, ma_uint64* pLoopPoint);
typedef ma_bool32 (* ma_data_source_at_end_proc)(ma_data_source* pDataSource);

// Data source virtual table.
typedef struct
{
    ma_data_source_read_proc        onRead;
    ma_data_source_seek_proc        onSeek;
    ma_data_source_get_cursor_proc  onGetCursor;
    ma_data_source_set_loop_point_proc onSetLoopPoint;
    ma_data_source_get_loop_point_proc onGetLoopPoint;
    ma_data_source_at_end_proc      onAtEnd;
} ma_data_source_vtable;

// Data source.
struct ma_data_source
{
    ma_data_source_vtable vtable;
    ma_format             format;
    ma_uint32             channelCount;
    ma_uint32             sampleRate;
    ma_uint64             cursorInPCMFrames;
    ma_uint64             loopPointInPCMFrames;
    ma_bool32             isLooping;
    ma_bool32             atEnd;
    void*                 pUserData;
};

// =============================================================================
// Decoded Audio Data
// =============================================================================
typedef struct ma_decoded_audio_data ma_decoded_audio_data;

// Decoded audio data.
struct ma_decoded_audio_data
{
    ma_format         format;
    ma_uint32         channelCount;
    ma_uint32         sampleRate;
    ma_uint64         frameCount;
    ma_uint64         totalSizeInBytes;  // Size in bytes of pData.
    void*             pData;             // Owned. Use ma_free() to free.
};

// =============================================================================
// Decoder Backend
// =============================================================================
typedef struct ma_decoder_backend ma_decoder_backend;

// Decoder backend virtual table.
typedef struct
{
    ma_result (* onInitFile)  (ma_decoder_backend* pBackend, const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
    ma_result (* onInitMemory)(ma_decoder_backend* pBackend, const void* pData, size_t dataSize, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
    ma_result (* onInitStream)(ma_decoder_backend* pBackend, ma_stream* pStream, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
    void      (* onUninit)    (ma_decoder_backend* pBackend);
    ma_result (* onReadPCMFrames) (ma_decoder_backend* pBackend, void* pBufferOut, ma_uint64 framesToRead, ma_uint64* pFramesRead);
    ma_result (* onSeekToPCMFrame) (ma_decoder_backend* pBackend, ma_uint64 frameIndex);
    ma_result (* onGetCursorInPCMFrames) (ma_decoder_backend* pBackend, ma_uint64* pCursor);
    ma_result (* onSetLoopPoint) (ma_decoder_backend* pBackend, ma_uint64 frameIndex);
    ma_result (* onGetLoopPoint) (ma_decoder_backend* pBackend, ma_uint64* pLoopPoint);
    ma_bool32 (* onAtEnd) (ma_decoder_backend* pBackend);
} ma_decoder_backend_vtable;

// Decoder backend.
struct ma_decoder_backend
{
    ma_decoder_backend_vtable vtable;
    void*                     pUserData;
};

// =============================================================================
// Decoder
// =============================================================================
typedef struct ma_decoder ma_decoder;

// Decoder config.
typedef struct
{
    ma_format         format;
    ma_uint32         channelCount;
    ma_uint32         sampleRate;
    ma_channel_map    channelMap;
} ma_decoder_config;

// Decoder.
struct ma_decoder
{
    ma_data_source        decoder;
    ma_decoder_backend*   pBackend;
    ma_decoder_config     config;
    ma_bool32             isStream;
    void*                 pUserData;
};

// =============================================================================
// Stream
// =============================================================================
typedef struct ma_stream ma_stream;

// Stream callback.
typedef ma_result (* ma_stream_read_proc)(ma_stream* pStream, void* pBuffer, ma_uint64 bytesToRead, ma_uint64* pBytesRead);
typedef ma_result (* ma_stream_seek_proc)(ma_stream* pStream, ma_int64 offset, ma_seek_origin origin);
typedef ma_result (* ma_stream_get_cursor_proc)(ma_stream* pStream, ma_int64* pCursor);

// Stream virtual table.
typedef struct
{
    ma_stream_read_proc     onRead;
    ma_stream_seek_proc     onSeek;
    ma_stream_get_cursor_proc onGetCursor;
} ma_stream_vtable;

// Stream.
struct ma_stream
{
    ma_stream_vtable vtable;
    ma_bool32        ownsMemory;
    ma_int64         cursor;
    ma_int64         size;
    void*            pData;      // Owned if ownsMemory is true.
};

// Seek origin.
typedef enum
{
    ma_seek_origin_start    = 0,
    ma_seek_origin_current  = 1,
    ma_seek_origin_end      = 2,
} ma_seek_origin;

// =============================================================================
// Logging
// =============================================================================
typedef enum
{
    ma_log_level_verbose   = 0,
    ma_log_level_info      = 1,
    ma_log_level_warning   = 2,
    ma_log_level_error     = 3,
} ma_log_level;

// Log callback.
typedef void (* ma_log_callback)(void* pUserData, ma_log_level level, const char* message);

// =============================================================================
// Initialization and Cleanup
// =============================================================================
ma_result ma_init(const ma_log_callback* pLogCallback, ma_allocation_callbacks* pAllocationCallbacks);
void ma_uninit();

// Version.
#define MA_VERSION_MAJOR   (0)
#define MA_VERSION_MINOR   (11)
#define MA_VERSION_REVISION (15)
#define MA_VERSION_STRING   MA_VERSION_MAJOR . MA_VERSION_MINOR . MA_VERSION_REVISION

// =============================================================================
// Device
// =============================================================================
ma_result ma_device_init(const ma_device_config* pConfig, ma_device* pDevice);
void ma_device_uninit(ma_device* pDevice);
ma_result ma_device_start(ma_device* pDevice);
ma_result ma_device_stop(ma_device* pDevice);
ma_result ma_device_set_master_volume(ma_device* pDevice, float volume);
ma_result ma_device_get_master_volume(ma_device* pDevice, float* pVolume);
ma_result ma_device_get_info(ma_device* pDevice, ma_device_info* pInfo);
ma_result ma_device_get_active_device_info(ma_device* pDevice, ma_device_info* pInfo);
ma_result ma_device_enumerate(ma_device_type deviceType, ma_device_callback_enum_callback callback, void* pUserData);
ma_result ma_device_get_default_format(ma_device_type deviceType, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate);
ma_result ma_device_get_default_buffer_size_in_milliseconds(ma_device_type deviceType, ma_uint32* pBufferSizeInMilliseconds);

// Device enumeration callback.
typedef ma_bool32 (* ma_device_callback_enum_callback)(const ma_device_info* pInfo, void* pUserData);

// =============================================================================
// Data Source
// =============================================================================
ma_result ma_data_source_init(ma_data_source* pDataSource, ma_format format, ma_uint32 channelCount, ma_uint32 sampleRate);
void ma_data_source_uninit(ma_data_source* pDataSource);
ma_result ma_data_source_read(ma_data_source* pDataSource, void* pBuffer, ma_uint64 framesToRead, ma_uint64* pFramesRead);
ma_result ma_data_source_seek(ma_data_source* pDataSource, ma_uint64 frameIndex);
ma_result ma_data_source_get_cursor(ma_data_source* pDataSource, ma_uint64* pCursor);
ma_result ma_data_source_set_loop_point(ma_data_source* pDataSource, ma_uint64 frameIndex);
ma_result ma_data_source_get_loop_point(ma_data_source* pDataSource, ma_uint64* pLoopPoint);
ma_bool32 ma_data_source_at_end(ma_data_source* pDataSource);

// =============================================================================
// Decoder Backend
// =============================================================================
ma_result ma_decoder_backend_init(const ma_decoder_backend_vtable* pVTable, ma_decoder_backend* pBackend);
void ma_decoder_backend_uninit(ma_decoder_backend* pBackend);
ma_result ma_decoder_backend_on_init_file(ma_decoder_backend* pBackend, const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_backend_on_init_memory(ma_decoder_backend* pBackend, const void* pData, size_t dataSize, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_backend_on_init_stream(ma_decoder_backend* pBackend, ma_stream* pStream, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_backend_on_read_pcm_frames(ma_decoder_backend* pBackend, void* pBufferOut, ma_uint64 framesToRead, ma_uint64* pFramesRead);
ma_result ma_decoder_backend_on_seek_to_pcm_frame(ma_decoder_backend* pBackend, ma_uint64 frameIndex);
ma_result ma_decoder_backend_on_get_cursor_in_pcm_frames(ma_decoder_backend* pBackend, ma_uint64* pCursor);
ma_result ma_decoder_backend_on_set_loop_point(ma_decoder_backend* pBackend, ma_uint64 frameIndex);
ma_result ma_decoder_backend_on_get_loop_point(ma_decoder_backend* pBackend, ma_uint64* pLoopPoint);
ma_bool32 ma_decoder_backend_on_at_end(ma_decoder_backend* pBackend);

// =============================================================================
// Decoder
// =============================================================================
ma_result ma_decoder_init_file(const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_init_memory(const void* pData, size_t dataSize, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
ma_result ma_decoder_init_stream(ma_stream* pStream, const ma_decoder_config* pConfig, ma_decoder* pDecoder);
void ma_decoder_uninit(ma_decoder* pDecoder);
ma_result ma_decoder_read_pcm_frames(ma_decoder* pDecoder, void* pBufferOut, ma_uint64 framesToRead, ma_uint64* pFramesRead);
ma_result ma_decoder_seek_to_pcm_frame(ma_decoder* pDecoder, ma_uint64 frameIndex);
ma_result ma_decoder_get_cursor_in_pcm_frames(ma_decoder* pDecoder, ma_uint64* pCursor);
ma_result ma_decoder_set_loop_point(ma_decoder* pDecoder, ma_uint64 frameIndex);
ma_result ma_decoder_get_loop_point(ma_decoder* pDecoder, ma_uint64* pLoopPoint);
ma_bool32 ma_decoder_at_end(ma_decoder* pDecoder);

// =============================================================================
// Stream
// =============================================================================
ma_result ma_stream_init(void* pData, size_t dataSize, ma_bool32 ownsMemory, ma_stream* pStream);
void ma_stream_uninit(ma_stream* pStream);
ma_result ma_stream_read(ma_stream* pStream, void* pBuffer, ma_uint64 bytesToRead, ma_uint64* pBytesRead);
ma_result ma_stream_seek(ma_stream* pStream, ma_int64 offset, ma_seek_origin origin);
ma_result ma_stream_get_cursor(ma_stream* pStream, ma_int64* pCursor);

// =============================================================================
// Resource Manager
// =============================================================================
typedef struct ma_resource_manager ma_resource_manager;
typedef struct ma_resource_manager_config ma_resource_manager_config;

// Resource manager.
struct ma_resource_manager
{
    // TODO: Implementation.
};

// Resource manager config.
struct ma_resource_manager_config
{
    // TODO: Implementation.
};

ma_result ma_resource_manager_init(const ma_resource_manager_config* pConfig, ma_resource_manager* pResourceManager);
void ma_resource_manager_uninit(ma_resource_manager* pResourceManager);
ma_result ma_resource_manager_data_source_init(ma_resource_manager* pResourceManager, const char* pFilePath, ma_data_source** ppDataSource);
ma_result ma_resource_manager_data_source_init_w(ma_resource_manager* pResourceManager, const wchar_t* pFilePath, ma_data_source** ppDataSource);
ma_result ma_resource_manager_data_source_init_from_memory(ma_resource_manager* pResourceManager, const void* pData, size_t dataSize, ma_data_source** ppDataSource);
void ma_resource_manager_data_source_uninit(ma_resource_manager* pResourceManager, ma_data_source* pDataSource);
ma_result ma_resource_manager_decode(ma_resource_manager* pResourceManager, const char* pFilePath, ma_format* pFormat, ma_uint32* pChannelCount, ma_uint32* pSampleRate, ma_uint64* pFrameCount, void** ppData);
ma_result ma_resource_manager_decode_w(ma_resource_manager* pResourceManager, const wchar_t* pFilePath, ma_format* pFormat, ma_uint32* pChannelCount, ma_uint32* pSampleRate, ma_uint64* pFrameCount, void** ppData);
ma_result ma_resource_manager_decode_from_memory(ma_resource_manager* pResourceManager, const void* pData, size_t dataSize, ma_format* pFormat, ma_uint32* pChannelCount, ma_uint32* pSampleRate, ma_uint64* pFrameCount, void** ppData);
ma_result ma_resource_manager_read_encoded(ma_resource_manager* pResourceManager, const char* pFilePath, void* pBuffer, ma_uint64 bytesToRead, ma_uint64* pBytesRead);
ma_result ma_resource_manager_read_encoded_w(ma_resource_manager* pResourceManager, const wchar_t* pFilePath, void* pBuffer, ma_uint64 bytesToRead, ma_uint64* pBytesRead);
ma_result ma_resource_manager_read_encoded_from_memory(ma_resource_manager* pResourceManager, const void* pData, size_t dataSize, void* pBuffer, ma_uint64 bytesToRead, ma_uint64* pBytesRead);

// =============================================================================
// Misc
// =============================================================================
void* ma_malloc(size_t size, ma_allocation_callbacks* pAllocationCallbacks);
void* ma_calloc(size_t count, size_t size, ma_allocation_callbacks* pAllocationCallbacks);
void  ma_free(void* p, ma_allocation_callbacks* pAllocationCallbacks);

// =============================================================================
// Allocation Callbacks
// =============================================================================
typedef struct
{
    void* (* onMalloc)(size_t size, void* pUserData);
    void* (* onCalloc)(size_t count, size_t size, void* pUserData);
    void  (* onFree)(void* p, void* pUserData);
} ma_allocation_callbacks;

// =============================================================================
// Helper Macros
// =============================================================================
#define ma_offset_of(type, member)   ((size_t)(&((type*)0)->member))
#define ma_container_of(ptr, type, member)   ((type*)(((char*)ptr) - ma_offset_of(type, member)))
#define ma_align_forward(ptr, align)   ((void*)(((uintptr_t)(ptr) + ((align) - 1)) & ~((uintptr_t)((align) - 1))))
#define ma_align_forward_offset(ptr, align)   (ma_align_forward(ptr, align) - ptr)
#define ma_align_backward(ptr, align)   ((void*)((uintptr_t)(ptr) & ~((uintptr_t)((align) - 1))))
#define ma_align_backward_offset(ptr, align)   (ptr - ma_align_backward(ptr, align))
#define ma_offset_align_forward(offset, align)   (((offset) + ((align) - 1)) & ~((align) - 1))
#define ma_offset_align_backward(offset, align)   ((offset) & ~((align) - 1))
#define ma_align_is_aligned(ptr, align)   ((((uintptr_t)ptr) & ((align) - 1)) == 0)
#define ma_is_bit_set(flags, bit)   (((flags) & ((ma_uint64)1 << (bit))) != 0)
#define ma_is_bit_clear(flags, bit)   (((flags) & ((ma_uint64)1 << (bit))) == 0)
#define ma_set_bit(flags, bit)   ((flags) |= ((ma_uint64)1 << (bit)))
#define ma_clear_bit(flags, bit)   ((flags) &= ~((ma_uint64)1 << (bit)))
#define ma_clamp(value, min, max)   (((value) < (min)) ? (min) : (((value) > (max)) ? (max) : (value)))
#define ma_saturate(value)   ma_clamp(value, 0.0f, 1.0f)
#define ma_len(arr)   (sizeof(arr) / sizeof(arr[0]))
#define ma_zero_object(p)   memset(&(p), 0, sizeof(p))
#define ma_zero_memory(p, sz)   memset((p), 0, (sz))
#define ma_copy_memory(pDst, pSrc, sz)   memcpy((pDst), (pSrc), (sz))
#define ma_move_memory(pDst, pSrc, sz)   memmove((pDst), (pSrc), (sz))
#define ma_zero_padding(sz)   (ma_align_forward_offset(sz, MA_CACHE_LINE) == 0 ? 0 : MA_CACHE_LINE - ma_align_forward_offset(sz, MA_CACHE_LINE))
#define MA_ALIGNMENT   16
#define MA_CACHE_LINE   64
#define MA_MAX_FILENAME_LENGTH   260
#define MA_MAX_PATH   260

#ifdef __cplusplus
}
#endif

#endif  // miniaudio_h