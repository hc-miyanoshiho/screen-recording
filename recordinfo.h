#ifndef RECORDINFO_H
#define RECORDINFO_H

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/version.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/fifo.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
}

#include <d3d11.h>
#include <dxgi1_6.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <endpointvolume.h>
#include <queue>
#include <QMutex>

namespace recorder
{
    enum RecordState
    {
        Recording,
        Finished
    };
}

typedef struct
{
    recorder::RecordState state;

    AVFormatContext *ifmt_ctx_v;
    AVCodecContext *icode_ctx_v;
    AVCodec *icode_v;

    HRESULT hr;
    ID3D11Device *d3d_device;
    ID3D11DeviceContext *d3d_device_ctx;
    DXGI_OUTPUT_DESC dxgi_output_desc;
    IDXGIOutputDuplication *dxgi_output_dupl;
    ID3D11Texture2D *no_mouse_image;
    D3D11_TEXTURE2D_DESC desc;
    DXGI_OUTDUPL_DESC dxgi_output_dupl_desc;
    IDXGIResource *dxgi_res;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    UINT subresource;
    AVRational fps;

    int size;
    struct SwsContext *sws_ctx;

    AVFormatContext *ofmt_ctx_v;
    AVCodec *ocode_v;
    AVCodecContext *ocode_ctx_v;
    AVStream *v_stream;
    int videoIndex_o;

    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    AVFilterGraph *filter_graph;
    const AVFilter *buffersrc;
    const AVFilter *buffersink;
    AVFilterInOut *outputs;
    AVFilterInOut *inputs;

    IAudioClient *audio_client_mic;
    IAudioCaptureClient *audio_capture_client_mic;
    WAVEFORMATEX *mic_format;
    DWORD mic_sleep_time;

    SwrContext *swr_ctx_mic;
    AVAudioFifo *audio_fifo_mic;
    AVFilterContext *abuffer_ctx1;

    IAudioClient *audio_client_render;
    IAudioRenderClient *audio_render_client;
    WAVEFORMATEX *pwfx;
    UINT32 frame_buffer_size;
    DWORD render_sleep_time;

    IAudioClient *audio_client_spk;
    IAudioCaptureClient *audio_capture_client_spk;
    WAVEFORMATEX *spk_format;
    DWORD spk_sleep_time;

    AVFormatContext *ofmt_ctx_a;
    AVCodec *ocode_a;
    AVCodecContext *ocode_ctx_a;
    AVStream *a_stream;
    int audioIndex_o;

    SwrContext *swr_ctx_spk;
    AVAudioFifo *audio_fifo_spk;
    AVFilterContext *abuffer_ctx2;

    AVFilterContext *sink_ctx;

    AVFilterGraph *graph;
    const AVFilter *filter1;
    const AVFilter *filter2;
    const AVFilter *amix;
    AVFilterContext *amix_ctx;
    const AVFilter *aformat;
    AVFilterContext *aformat_ctx;
    const AVFilter *sink;

    QMutex mutex;
} record_t;

typedef struct
{
    std::queue<AVFrame*> video_frames;
} queue_t;

#endif // RECORDINFO_H
