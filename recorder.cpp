#include "recorder.h"
#include <QDebug>

#define FREE_VIDEO_GDI_IN \
if (r.icode_ctx_v)\
{\
    avcodec_close(r.icode_ctx_v);\
    avcodec_free_context(&r.icode_ctx_v);\
}\
if (r.ifmt_ctx_v)\
    avformat_close_input(&r.ifmt_ctx_v);

#define FREE_VIDEO_DXGI_IN \
if (r.d3d_device_ctx)\
{\
    r.d3d_device_ctx->Release();\
    r.d3d_device_ctx = nullptr;\
}\
if (r.dxgi_output_dupl)\
{\
    r.dxgi_output_dupl->ReleaseFrame();\
    r.dxgi_output_dupl->Release();\
    r.dxgi_output_dupl = nullptr;\
}

#define FREE_VIDEO_OUT \
if (r.sws_ctx)\
{\
    sws_freeContext(r.sws_ctx);\
    r.sws_ctx = nullptr;\
}\
if (r.ocode_ctx_v)\
{\
    avcodec_close(r.ocode_ctx_v);\
    avcodec_free_context(&r.ocode_ctx_v);\
}\
if (r.ofmt_ctx_v)\
{\
    avformat_free_context(r.ofmt_ctx_v);\
    r.ofmt_ctx_v = nullptr;\
}

#define FREE_VIDEO_FILTER \
if (r.buffersrc_ctx)\
    avfilter_free(r.buffersrc_ctx);\
if (r.buffersink_ctx)\
    avfilter_free(r.buffersink_ctx);\
if (r.inputs)\
    avfilter_inout_free(&r.inputs);\
if (r.outputs)\
    avfilter_inout_free(&r.outputs);\
if (r.filter_graph)\
    avfilter_graph_free(&r.filter_graph);

#define FREE_MIC_IN \
if (r.audio_capture_client_mic)\
{\
    r.audio_capture_client_mic->Release();\
    r.audio_capture_client_mic = nullptr;\
}\
if (r.mic_format)\
{\
    CoTaskMemFree(r.mic_format);\
    r.mic_format = nullptr;\
}\
if (r.audio_client_mic)\
{\
    r.audio_client_mic->Stop();\
    r.audio_client_mic->Release();\
    r.audio_client_mic = nullptr;\
}

#define FREE_SPK_RENDER \
if (r.audio_render_client)\
{\
    r.audio_render_client->Release();\
    r.audio_render_client = nullptr;\
}\
if (r.pwfx)\
{\
    CoTaskMemFree(r.pwfx);\
    r.pwfx = nullptr;\
}\
if (r.audio_client_render)\
{\
    r.audio_client_render->Stop();\
    r.audio_client_render->Release();\
    r.audio_client_render = nullptr;\
}

#define FREE_SPK_IN \
if (r.audio_capture_client_spk)\
{\
    r.audio_capture_client_spk->Release();\
    r.audio_capture_client_spk = nullptr;\
}\
if (r.spk_format)\
{\
    CoTaskMemFree(r.spk_format);\
    r.spk_format = nullptr;\
}\
if (r.audio_client_spk)\
{\
    r.audio_client_spk->Stop();\
    r.audio_client_spk->Release();\
    r.audio_client_spk = nullptr;\
}

#define FREE_AUDIO_OUT \
if (r.swr_ctx_mic)\
    swr_free(&r.swr_ctx_mic);\
if (r.audio_fifo_mic)\
{\
    av_audio_fifo_free(r.audio_fifo_mic);\
    r.audio_fifo_mic = nullptr;\
}\
if (r.swr_ctx_spk)\
    swr_free(&r.swr_ctx_spk);\
if (r.audio_fifo_spk)\
{\
    av_audio_fifo_free(r.audio_fifo_spk);\
    r.audio_fifo_spk = nullptr;\
}\
if (r.ocode_ctx_a)\
{\
    avcodec_close(r.ocode_ctx_a);\
    avcodec_free_context(&r.ocode_ctx_a);\
}\
if (r.ofmt_ctx_a)\
{\
    avformat_free_context(r.ofmt_ctx_a);\
    r.ofmt_ctx_a = nullptr;\
}

#define FREE_AUDIO_FILTER \
if (r.graph)\
    avfilter_graph_free(&r.graph);

using namespace recorder;

Recorder::Recorder(int mode, QObject *parent) : QObject(parent), mode(mode)
{
    qDebug() << "mode = " << mode;
}

int Recorder::initGDIVideoInput()
{
    int result = 0;
    do
    {
        r.ifmt_ctx_v = nullptr;
        r.icode_ctx_v = nullptr;
        AVInputFormat *ifmt = av_find_input_format("gdigrab");
        if (!ifmt)
        {
            result = 0x01;
            break;
        }
        if (avformat_open_input(&r.ifmt_ctx_v, "desktop", ifmt, nullptr) < 0)
        {
            result = 0x02;
            break;
        }
        if (avformat_find_stream_info(r.ifmt_ctx_v, nullptr) < 0)
        {
            result = 0x03;
            break;
        }
        int video_index = -1;
        for (uint i = 0; i < r.ifmt_ctx_v->nb_streams; ++i)
        {
            if (r.ifmt_ctx_v->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                video_index = i;
                break;
            }
        }
        if (video_index == -1)
        {
            result = 0x04;
            break;
        }
        r.icode_v = avcodec_find_decoder(r.ifmt_ctx_v->streams[video_index]->codecpar->codec_id);
        if (!r.icode_v)
        {
            result = 0x05;
            break;
        }
        r.icode_ctx_v = avcodec_alloc_context3(r.icode_v);
        if (!r.icode_ctx_v)
        {
            result = 0x06;
            break;
        }
        if (avcodec_parameters_to_context(r.icode_ctx_v, r.ifmt_ctx_v->streams[video_index]->codecpar) < 0)
        {
            result = 0x07;
            break;
        }
        if (avcodec_open2(r.icode_ctx_v, r.icode_v, nullptr) < 0)
        {
            result = 0x08;
            break;
        }
    }
    while (0);
    return result;
}

int Recorder::initDXGIVideoInput()
{
    int result = 0;
    do
    {
        D3D_DRIVER_TYPE driver_types[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP
        };
        UINT driver_types_num = ARRAYSIZE(driver_types);
        D3D_FEATURE_LEVEL feature_levels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        UINT feature_levels_num = ARRAYSIZE(feature_levels);
        r.d3d_device = nullptr;
        r.d3d_device_ctx = nullptr;
        r.dxgi_output_dupl = nullptr;
        r.no_mouse_image = nullptr;
        r.dxgi_res = nullptr;
        D3D_FEATURE_LEVEL feature_level;
        for (UINT DriverTypeIndex = 0; DriverTypeIndex < driver_types_num; ++DriverTypeIndex)
        {
            r.hr = D3D11CreateDevice(nullptr, driver_types[DriverTypeIndex], nullptr, 0, feature_levels, feature_levels_num, D3D11_SDK_VERSION,
                &r.d3d_device, &feature_level, &r.d3d_device_ctx);
            if (SUCCEEDED(r.hr))
                break;
            r.d3d_device->Release();
            r.d3d_device_ctx->Release();
        }
        if (FAILED(r.hr))
        {
            result = 0x01;
            break;
        }
        if (!r.d3d_device)
        {
           result = 0x02;
           break;
        }
        IDXGIDevice *dxgi_device = nullptr;
        r.hr = r.d3d_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
        if (FAILED(r.hr))
        {
            result = 0x03;
            break;
        }
        r.d3d_device->AddRef();
        IDXGIDevice *DxgiDevice = nullptr;
        r.hr = r.d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
        if (FAILED(r.hr))
        {
            result = 0x04;
            break;
        }
        IDXGIAdapter *DxgiAdapter = nullptr;
        r.hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
        DxgiDevice->Release();
        DxgiDevice = nullptr;
        if (FAILED(r.hr))
        {
            result = 0x05;
            break;
        }
        UINT Output = 0;
        IDXGIOutput *DxgiOutput = nullptr;
        r.hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
        DxgiAdapter->Release();
        DxgiAdapter = nullptr;
        if (FAILED(r.hr))
        {
            result = 0x06;
            break;
        }
        DxgiOutput->GetDesc(&r.dxgi_output_desc);
        IDXGIOutput1 *DxgiOutput1 = nullptr;
        r.hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
        DxgiOutput->Release();
        DxgiOutput = nullptr;
        if (FAILED(r.hr))
        {
            result = 0x07;
            break;
        }
        r.hr = DxgiOutput1->DuplicateOutput(r.d3d_device, &r.dxgi_output_dupl);
        DxgiOutput1->Release();
        DxgiOutput1 = nullptr;
        if (FAILED(r.hr))
        {
            if (r.hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                result = 0x08;
                break;
            }
            result = 0x09;
            break;
        }
        r.subresource = D3D11CalcSubresource(0, 0, 0);
        r.dxgi_output_dupl->GetDesc(&r.dxgi_output_dupl_desc);
        r.desc.Width = r.dxgi_output_dupl_desc.ModeDesc.Width;
        r.desc.Height = r.dxgi_output_dupl_desc.ModeDesc.Height;
        r.desc.Format = r.dxgi_output_dupl_desc.ModeDesc.Format;
        r.desc.ArraySize = 1;
        r.desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
        r.desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
        r.desc.SampleDesc.Count = 1;
        r.desc.SampleDesc.Quality = 0;
        r.desc.MipLevels = 1;
        r.desc.CPUAccessFlags = 0;
        r.desc.Usage = D3D11_USAGE_DEFAULT;

        r.desc.Width = r.dxgi_output_dupl_desc.ModeDesc.Width;
        r.desc.Height = r.dxgi_output_dupl_desc.ModeDesc.Height;
        r.desc.Format = r.dxgi_output_dupl_desc.ModeDesc.Format;
        r.desc.ArraySize = 1;
        r.desc.BindFlags = 0;
        r.desc.MiscFlags = 0;
        r.desc.SampleDesc.Count = 1;
        r.desc.SampleDesc.Quality = 0;
        r.desc.MipLevels = 1;
        r.desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        r.desc.Usage = D3D11_USAGE_STAGING;
    }
    while (0);
    return result;
}

int Recorder::initVideoOutput()
{
    int result = 0;
    do
    {
        r.ocode_ctx_v = nullptr;
        r.sws_ctx = nullptr;
        r.ocode_v = avcodec_find_encoder(AV_CODEC_ID_H264);
        r.v_stream = avformat_new_stream(r.ofmt_ctx_v, r.ocode_v);
        if (!r.v_stream)
        {
            result = 0x21;
            break;
        }
        r.videoIndex_o = r.v_stream->index;
        r.ocode_ctx_v = avcodec_alloc_context3(r.ocode_v);
        if (!r.ocode_ctx_v)
        {
            result = 0x22;
            break;
        }
        if (mode == 1)
        {
            r.ocode_ctx_v->width = r.desc.Width;
            r.ocode_ctx_v->height = r.desc.Height;
        }
        else
        {
            r.ocode_ctx_v->width = r.icode_ctx_v->width;
            r.ocode_ctx_v->height = r.icode_ctx_v->height;
        }
        r.ocode_ctx_v->pix_fmt = AV_PIX_FMT_YUV420P;
        r.ocode_ctx_v->time_base = { r.fps.den, r.fps.num };
        r.ocode_ctx_v->framerate = r.fps;
        r.ocode_ctx_v->max_b_frames = 0;
        r.ocode_ctx_v->thread_count = 2;
        if (r.ofmt_ctx_v->oformat->flags & AVFMT_GLOBALHEADER)
            r.ocode_ctx_v->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        AVDictionary *opt = nullptr;
        av_dict_set(&opt, "tune", "zerolatency", 0);
        av_dict_set(&opt, "preset", "veryfast", 0);
        av_dict_set(&opt, "profile", "baseline", 0);
        av_dict_set(&opt, "level", "5.2", 0);
    //    av_dict_set_int(&opt, "crf", 20, 0);
        if (avcodec_open2(r.ocode_ctx_v, r.ocode_v, &opt))
        {
            result = 0x23;
            break;
        }
        if (avcodec_parameters_from_context(r.v_stream->codecpar, r.ocode_ctx_v))
        {
            result = 0x24;
            break;
        }
        r.v_stream->time_base = { r.fps.den, r.fps.num };
        r.size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, r.ocode_ctx_v->width, r.ocode_ctx_v->height, 1);
        if (mode == 1)
            r.sws_ctx = sws_getContext(r.desc.Width, r.desc.Height, AV_PIX_FMT_BGRA, r.ocode_ctx_v->width,
                                    r.ocode_ctx_v->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
        else
            r.sws_ctx = sws_getContext(r.icode_ctx_v->width, r.icode_ctx_v->height, AV_PIX_FMT_BGRA, r.ocode_ctx_v->width,
                                    r.ocode_ctx_v->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!r.sws_ctx)
        {
            result = 0x25;
            break;
        }
    }
    while (0);
    return result;
}

int Recorder::initMicInput()
{
    int result = 0;
    do
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (hr == S_FALSE)
        {
            result = 0x41;
            break;
        }
        IMMDevice* device_mic = nullptr;
        IMMDeviceEnumerator *device_enumerator_mic = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator_mic);
        if (FAILED(hr))
        {
            result = 0x42;
            break;
        }
        device_enumerator_mic->GetDefaultAudioEndpoint(eCapture, eCommunications, &device_mic);
        device_enumerator_mic->Release();
        IAudioEndpointVolume *mic_volume = nullptr;
        hr = device_mic->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&mic_volume);
        if(FAILED(hr)){
            result = 0x43;
            break;
        }
        float range_min, range_max, range_increment;
        hr = mic_volume->GetVolumeRange(&range_min, &range_max, &range_increment);
        if(FAILED(hr)){
            result = 0x44;
            break;
        }
        hr = mic_volume->SetMasterVolumeLevel(range_max, nullptr);
        if(FAILED(hr)){
            result = 0x45;
            break;
        }
        r.audio_client_mic = nullptr;
        hr = device_mic->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&r.audio_client_mic));
        if (FAILED(hr))
        {
            result = 0x46;
            break;
        }
        r.audio_capture_client_mic = nullptr;
        r.mic_format = nullptr;
        hr = r.audio_client_mic->GetMixFormat(&r.mic_format);
        if (FAILED(hr))
        {
            result = 0x47;
            break;
        }
        if (r.mic_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            r.mic_format->wFormatTag = WAVE_FORMAT_PCM;
            if (r.mic_format->nChannels > 2)
                r.mic_format->nChannels = 2;
            r.mic_format->wBitsPerSample = 16;
            r.mic_format->nBlockAlign = r.mic_format->nChannels * r.mic_format->wBitsPerSample / 8;
            r.mic_format->nAvgBytesPerSec = r.mic_format->nBlockAlign * r.mic_format->nSamplesPerSec;
        }
        else if (r.mic_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(r.mic_format);
            if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
            {
                pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                pEx->Samples.wValidBitsPerSample = 16;
                if (r.mic_format->nChannels > 2)
                    r.mic_format->nChannels = 2;
                r.mic_format->wBitsPerSample = 16;
                r.mic_format->nBlockAlign = r.mic_format->nChannels * r.mic_format->wBitsPerSample / 8;
                r.mic_format->nAvgBytesPerSec = r.mic_format->nBlockAlign * r.mic_format->nSamplesPerSec;
            }
            else
            {
                result = 0x48;
                break;
            }
        }
        else
        {
            result = 0x49;
            break;
        }
        hr = r.audio_client_mic->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, r.mic_format, NULL);
        if (FAILED(hr))
        {
            result = 0x4A;
            break;
        }
        UINT32 bufferFrameCount;
        r.audio_client_mic->GetBufferSize(&bufferFrameCount);
        hr = r.audio_client_mic->GetService(__uuidof(IAudioCaptureClient), (void**)&r.audio_capture_client_mic);
        if (FAILED(hr))
        {
            result = 0x4B;
            break;
        }
        hr = r.audio_client_mic->Start();
        if (FAILED(hr))
        {
            result = 0x4C;
            break;
        }
        if (r.mic_format->wFormatTag != WAVE_FORMAT_PCM && r.mic_format->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        {
            result = 0x4D;
            break;
        }
        r.mic_sleep_time = 500 * bufferFrameCount / r.mic_format->nSamplesPerSec;
    }
    while (0);
    return result;
}

int Recorder::initSpkRender()
{
    int result = 0;
    do
    {
        r.audio_client_render = nullptr;
        r.audio_render_client = nullptr;
        r.pwfx = nullptr;
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (hr == S_FALSE)
        {
            result = 0x61;
            break;
        }
        IMMDevice* device = nullptr;
        IMMDeviceEnumerator *device_enumerator = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
        if (FAILED(hr))
        {
            result = 0x62;
            break;
        }
        hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr))
        {
            result = 0x63;
            break;
        }
        device_enumerator->Release();
        r.frame_buffer_size = 0;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&r.audio_client_render);
        if (FAILED(hr))
        {
            result = 0x64;
            break;
        }
        hr = r.audio_client_render->GetMixFormat(&r.pwfx);
        if (FAILED(hr))
        {
            result = 0x65;
            break;
        }
        hr = r.audio_client_render->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, r.pwfx, nullptr);
        if (FAILED(hr))
        {
            result = 0x66;
            break;
        }
        hr = r.audio_client_render->GetBufferSize(&r.frame_buffer_size);
        if (FAILED(hr))
        {
            result = 0x67;
            break;
        }
        hr = r.audio_client_render->GetService(__uuidof(IAudioRenderClient), (void**)&r.audio_render_client);
        if (FAILED(hr))
        {
            result = 0x68;
            break;
        }
        BYTE *pData = nullptr;
        hr = r.audio_render_client->GetBuffer(r.frame_buffer_size, &pData);
        if (FAILED(hr))
        {
            result = 0x69;
            break;
        }
        hr = r.audio_render_client->ReleaseBuffer(r.frame_buffer_size, 0);
        if (FAILED(hr))
        {
            result = 0x6A;
            break;
        }
        hr = r.audio_client_render->Start();
        if (FAILED(hr))
        {
            result = 0x6B;
            break;
        }
        r.render_sleep_time = 500 * r.frame_buffer_size / r.pwfx->nSamplesPerSec;
//        r.render_sleep_time = 10000000.0 * r.frame_buffer_size / r.pwfx->nSamplesPerSec / 10000 / 2;
    }
    while (0);
    return result;
}

int Recorder::initSpkInput()
{
    int result = 0;
    do
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (hr == S_FALSE)
        {
            result = 0x81;
            break;
        }
        IMMDeviceEnumerator *device_enumerator_spk = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator_spk);
        if (FAILED(hr))
        {
            result = 0x82;
            break;
        }
        IMMDevice* device_spk = nullptr;
        device_enumerator_spk->GetDefaultAudioEndpoint(eRender, eConsole, &device_spk);
        device_enumerator_spk->Release();
        r.audio_client_spk = nullptr;
        hr = device_spk->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&r.audio_client_spk);
        if (FAILED(hr))
        {
            result = 0x83;
            break;
        }
        REFERENCE_TIME default_device_period = 0;
        hr = r.audio_client_spk->GetDevicePeriod(&default_device_period, nullptr);
        if (FAILED(hr))
        {
            result = 0x84;
            break;
        }
        r.spk_format = nullptr;
        hr = r.audio_client_spk->GetMixFormat(&r.spk_format);
        if (FAILED(hr))
        {
            result = 0x85;
            break;
        }
        if (r.spk_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            r.spk_format->wFormatTag = WAVE_FORMAT_PCM;
            if (r.spk_format->nChannels > 2)
                r.spk_format->nChannels = 2;
            r.spk_format->wBitsPerSample = 16;
            r.spk_format->nBlockAlign = r.spk_format->nChannels * r.spk_format->wBitsPerSample / 8;
            r.spk_format->nAvgBytesPerSec = r.spk_format->nBlockAlign * r.spk_format->nSamplesPerSec;
        }
        else if (r.spk_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(r.spk_format);
            if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
            {
                pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                pEx->Samples.wValidBitsPerSample = 16;
                if (r.spk_format->nChannels > 2)
                    r.spk_format->nChannels = 2;
                r.spk_format->wBitsPerSample = 16;
                r.spk_format->nBlockAlign = r.spk_format->nChannels * r.spk_format->wBitsPerSample / 8;
                r.spk_format->nAvgBytesPerSec = r.spk_format->nBlockAlign * r.spk_format->nSamplesPerSec;
            }
            else
            {
                result = 0x86;
                break;
            }
        }
        else
        {
            result = 0x87;
            break;
        }
        hr = r.audio_client_spk->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, r.spk_format, nullptr);
        if (FAILED(hr))
        {
            result = 0x88;
            break;
        }
        UINT32 bufferFrameCount;
        r.audio_client_spk->GetBufferSize(&bufferFrameCount);
        r.audio_capture_client_spk = nullptr;
        hr = r.audio_client_spk->GetService(__uuidof(IAudioCaptureClient), (void**)&r.audio_capture_client_spk);
        if (FAILED(hr))
        {
            result = 0x89;
            break;
        }
        hr = r.audio_client_spk->Start();
        if (FAILED(hr))
        {
            result = 0x8A;
            break;
        }
        if (r.spk_format->wFormatTag != WAVE_FORMAT_PCM && r.spk_format->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
        {
            result = 0x8B;
            break;
        }
        r.spk_sleep_time = 500 * bufferFrameCount / r.spk_format->nSamplesPerSec;
    }
    while (0);
    return result;
}

int Recorder::initAudioOutput()
{
    int result = 0;
    do
    {
        r.ocode_ctx_a = nullptr;
        r.swr_ctx_mic = nullptr;
        r.swr_ctx_spk = nullptr;
        r.audio_fifo_mic = nullptr;
        r.audio_fifo_spk = nullptr;
        r.ocode_a = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!r.ocode_a)
        {
            result = 0xA1;
            break;
        }
        r.ocode_ctx_a = avcodec_alloc_context3(r.ocode_a);
        if (!r.ocode_ctx_a)
        {
            result = 0xA2;
            break;
        }
        r.ocode_ctx_a->sample_fmt = AV_SAMPLE_FMT_FLTP;
        r.ocode_ctx_a->sample_rate = 48000;
        r.ocode_ctx_a->channel_layout = AV_CH_LAYOUT_STEREO;
        r.ocode_ctx_a->channels = av_get_channel_layout_nb_channels(r.ocode_ctx_a->channel_layout);
        r.ocode_ctx_a->time_base = {1, r.ocode_ctx_a->sample_rate};
        r.ocode_ctx_a->bit_rate = r.mic_format->nAvgBytesPerSec * 8 + r.spk_format->nAvgBytesPerSec * 8;
        r.ocode_ctx_a->thread_count = 1;
        if (r.ofmt_ctx_a->oformat->flags & AVFMT_GLOBALHEADER)
            r.ocode_ctx_a->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        AVDictionary *opt = nullptr;
        av_dict_set(&opt, "tune", "zerolatency", 0);
        if (avcodec_open2(r.ocode_ctx_a, r.ocode_a, &opt) < 0)
        {
            result = 0xA3;
            break;
        }
        r.a_stream = avformat_new_stream(r.ofmt_ctx_a, r.ocode_a);
        if (!r.a_stream)
        {
            result = 0xA4;
            break;
        }
        r.audioIndex_o = r.a_stream->index;
        if (avcodec_parameters_from_context(r.a_stream->codecpar, r.ocode_ctx_a) < 0)
        {
            result = 0xA5;
            break;
        }
        r.a_stream->time_base = r.ocode_ctx_a->time_base;
        r.swr_ctx_mic = swr_alloc();
        av_opt_set_int(r.swr_ctx_mic, "in_channel_count", r.mic_format->nChannels, 0);
        av_opt_set_int(r.swr_ctx_mic, "in_sample_rate", r.mic_format->nSamplesPerSec, 0);
        av_opt_set_sample_fmt(r.swr_ctx_mic, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int(r.swr_ctx_mic, "out_channel_count", r.ocode_ctx_a->channels, 0);
        av_opt_set_int(r.swr_ctx_mic, "out_sample_rate", r.ocode_ctx_a->sample_rate, 0);
        av_opt_set_sample_fmt(r.swr_ctx_mic, "out_sample_fmt", r.ocode_ctx_a->sample_fmt, 0);
        if (swr_init(r.swr_ctx_mic))
        {
            result = 0xA6;
            break;
        }
        r.audio_fifo_mic = av_audio_fifo_alloc(r.ocode_ctx_a->sample_fmt, r.ocode_ctx_a->channels, 1);
        r.swr_ctx_spk = swr_alloc();
        av_opt_set_int(r.swr_ctx_spk, "in_channel_count", r.spk_format->nChannels, 0);
        av_opt_set_int(r.swr_ctx_spk, "in_sample_rate", r.spk_format->nSamplesPerSec, 0);
        av_opt_set_sample_fmt(r.swr_ctx_spk, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int(r.swr_ctx_spk, "out_channel_count", r.ocode_ctx_a->channels, 0);
        av_opt_set_int(r.swr_ctx_spk, "out_sample_rate", r.ocode_ctx_a->sample_rate, 0);
        av_opt_set_sample_fmt(r.swr_ctx_spk, "out_sample_fmt", r.ocode_ctx_a->sample_fmt, 0);
        if (swr_init(r.swr_ctx_spk))
        {
            result = 0xA7;
            break;
        }
        r.audio_fifo_spk = av_audio_fifo_alloc(r.ocode_ctx_a->sample_fmt, r.ocode_ctx_a->channels, 1);
    }
    while (0);
    return result;
}

int Recorder::initAudioFilter()
{
    int result = 0;
    do
    {
        r.graph = nullptr;
        r.amix_ctx = nullptr;
        r.aformat_ctx = nullptr;
        r.graph = avfilter_graph_alloc();
        r.filter1 = avfilter_get_by_name("abuffer");
        r.filter2 = avfilter_get_by_name("abuffer");
        const char *name1 = "src0";
        r.abuffer_ctx1 = avfilter_graph_alloc_filter(r.graph, r.filter1, name1);
        if (avfilter_init_str(r.abuffer_ctx1, "sample_rate=48000:sample_fmt=fltp:channel_layout=3") < 0)
        {
            result = 0xC1;
            break;
        }
        const char *name2 = "src1";
        r.abuffer_ctx2 = avfilter_graph_alloc_filter(r.graph, r.filter2, name2);
        if (avfilter_init_str(r.abuffer_ctx2, "sample_rate=48000:sample_fmt=fltp:channel_layout=3") < 0)
        {
            result = 0xC2;
            break;
        }
        r.amix = avfilter_get_by_name("amix");
        r.amix_ctx = avfilter_graph_alloc_filter(r.graph, r.amix, "amix");
        const char *args = "inputs=2:duration=first:dropout_transition=3";
        if (avfilter_init_str(r.amix_ctx, args) < 0)
        {
            result = 0xC3;
            break;
        }
        r.aformat = avfilter_get_by_name("aformat");
        r.aformat_ctx = avfilter_graph_alloc_filter(r.graph, r.aformat, "aformat");
        if (avfilter_init_str(r.aformat_ctx, "sample_rates=48000:sample_fmts=fltp:channel_layouts=3") < 0)
        {
            result = 0xC4;
            break;
        }
        r.sink = avfilter_get_by_name("abuffersink");
        r.sink_ctx = avfilter_graph_alloc_filter(r.graph, r.sink, "sink");
        avfilter_init_str(r.sink_ctx, nullptr);
        if (avfilter_link(r.abuffer_ctx1, 0, r.amix_ctx, 0))
        {
            result = 0xC5;
            break;
        }
        if (avfilter_link(r.abuffer_ctx2, 0, r.amix_ctx, 1))
        {
            result = 0xC6;
            break;
        }
        if (avfilter_link(r.amix_ctx, 0, r.aformat_ctx, 0) < 0)
        {
            result = 0xC7;
            break;
        }
        if (avfilter_link(r.aformat_ctx, 0, r.sink_ctx, 0) < 0)
        {
            result = 0xC8;
            break;
        }
        if (avfilter_graph_config(r.graph, nullptr) < 0)
        {
            result = 0xC9;
            break;
        }
    }
    while (0);
    return result;
}

int Recorder::initVideoFilter(){
    r.inputs = nullptr;
    r.outputs = nullptr;
    r.filter_graph = nullptr;
    //1920 x 1080 crop 1280 x 720 and add a water_logo
    QString str = "[in]crop=1280:720:320:180[tmp];movie=water_logo.png[waterlogo];[tmp][waterlogo]overlay=1100:20[out]";
    int ret = 0;
    r.buffersrc  = avfilter_get_by_name("buffer");
    r.buffersink = avfilter_get_by_name("buffersink");
    r.outputs = avfilter_inout_alloc();
    r.inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE};
    r.filter_graph = avfilter_graph_alloc();
    if (!r.outputs || !r.inputs || !r.filter_graph)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF1;
    }
    QString args = "video_size=1280x720:pix_fmt=" +  QString::number(AV_PIX_FMT_BGRA) + ":time_base=1/30";
    ret = avfilter_graph_create_filter(&r.buffersrc_ctx, r.buffersrc, "in", args.toStdString().c_str(), nullptr, r.filter_graph);
    if (ret < 0)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF2;
    }
    ret = avfilter_graph_create_filter(&r.buffersink_ctx, r.buffersink, "out", nullptr, nullptr, r.filter_graph);
    if (ret < 0)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF3;
    }
    ret = av_opt_set_int_list(r.buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF4;
    }
    r.outputs->name = av_strdup("in");
    r.outputs->filter_ctx = r.buffersrc_ctx;
    r.outputs->pad_idx = 0;
    r.outputs->next = nullptr;
    r.inputs->name = av_strdup("out");
    r.inputs->filter_ctx = r.buffersink_ctx;
    r.inputs->pad_idx = 0;
    r.inputs->next = nullptr;
    if (avfilter_graph_parse_ptr(r.filter_graph, str.toStdString().c_str(), &r.inputs, &r.outputs, nullptr) < 0)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF5;
    }
    if (avfilter_graph_config(r.filter_graph, nullptr) < 0)
    {
        avfilter_inout_free(&r.inputs);
        avfilter_inout_free(&r.outputs);
        return 0xF6;
    }
    return 0x00;
}

void Recorder::record()
{
    render_spk = new RenderSpk(r);
    mix_audio = new MixAudio(r);
    record_video = new RecordVideo(r, q);
    write_video = new WriteVideo(r, q, mode);
    render_thread = new QThread;
    mix_thread = new QThread;
    video_thread = new QThread;
    write_thread = new QThread;
    render_spk->moveToThread(render_thread);
    mix_audio->moveToThread(mix_thread);
    record_video->moveToThread(video_thread);
    write_video->moveToThread(write_thread);
    connect(render_thread, SIGNAL(started()), render_spk, SLOT(render()));
    connect(mix_thread, SIGNAL(started()), mix_audio, SLOT(mix()));
    if (mode == 1)
        connect(video_thread, SIGNAL(started()), record_video, SLOT(record_dxgi()));
    else
        connect(video_thread, SIGNAL(started()), record_video, SLOT(record_gdi()));
    connect(write_thread, SIGNAL(started()), write_video, SLOT(write()));
    connect(render_spk, SIGNAL(over()), this, SLOT(finished()));
    connect(mix_audio, SIGNAL(write_finished()), this, SLOT(finished()));
    connect(write_video, SIGNAL(write_finished()), this, SLOT(finished()));
    r.state = RecordState::Recording;
    render_thread->start();
    mix_thread->start();
    video_thread->start();
    write_thread->start();
    emit started();
}

void Recorder::save()
{
    AVFormatContext *ifmt_ctx_v = nullptr, *ifmt_ctx_a = nullptr, *ofmt_ctx = nullptr;
    int video_index = -1, video_index_o = -1, audio_index = -1, audio_index_o = -1;
    int64_t cur_pts_v = 0, cur_pts_a = 0;
    if (avformat_open_input(&ifmt_ctx_v, video_filename.toStdString().c_str(), nullptr, nullptr) < 0)
        return;
    if (avformat_find_stream_info(ifmt_ctx_v, nullptr) < 0)
        return;
    if (avformat_open_input(&ifmt_ctx_a, audio_filename.toStdString().c_str(), nullptr, nullptr) < 0)
        return;
    if (avformat_find_stream_info(ifmt_ctx_a, nullptr) < 0)
        return;
    if (avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, filename.toStdString().c_str()) < 0)
        return;
    for (int i = 0; i < (int)ifmt_ctx_v->nb_streams; ++i)
    {
        if (ifmt_ctx_v->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }
    if (video_index == -1)
        return;
    AVStream *v_stream = avformat_new_stream(ofmt_ctx, avcodec_find_encoder(ifmt_ctx_v->streams[video_index]->codecpar->codec_id));
    if (!v_stream)
        return;
    video_index_o = v_stream->index;
    if (avcodec_parameters_copy(v_stream->codecpar, ifmt_ctx_v->streams[video_index]->codecpar) < 0)
        return;
    for (int i = 0; i < (int)ifmt_ctx_a->nb_streams; ++i)
    {
        if (ifmt_ctx_a->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_index = i;
            break;
        }
    }
    if (audio_index == -1)
        return;
    AVStream *a_stream = avformat_new_stream(ofmt_ctx, avcodec_find_encoder(ifmt_ctx_a->streams[audio_index]->codecpar->codec_id));
    if (!a_stream)
        return;
    audio_index_o = a_stream->index;
    if (avcodec_parameters_copy(a_stream->codecpar, ifmt_ctx_a->streams[audio_index]->codecpar) < 0)
        return;
    if (ofmt_ctx->oformat->flags & AVFMT_NOFILE)
        return;
    if (avio_open(&ofmt_ctx->pb, filename.toStdString().c_str(), AVIO_FLAG_READ_WRITE) < 0)
        return;
    if (avformat_write_header(ofmt_ctx, NULL) < 0)
        return;
    AVPacket *packet = nullptr;
    while (1)
    {
        if (av_compare_ts(cur_pts_v, ifmt_ctx_v->streams[video_index]->time_base, cur_pts_a, ifmt_ctx_a->streams[audio_index]->time_base) <= 0)
        {
            packet = av_packet_alloc();
            if (av_read_frame(ifmt_ctx_v, packet) < 0)
            {
                av_packet_free(&packet);
                break;
            }
            if (packet->stream_index == video_index)
            {
                cur_pts_v = packet->pts;
                packet->pts = av_rescale_q_rnd(packet->pts, ifmt_ctx_v->streams[video_index]->time_base, ofmt_ctx->streams[video_index_o]->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                packet->dts = av_rescale_q_rnd(packet->dts, ifmt_ctx_v->streams[video_index]->time_base, ofmt_ctx->streams[video_index_o]->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                packet->duration = av_rescale_q(packet->duration, ifmt_ctx_v->streams[video_index]->time_base, ofmt_ctx->streams[video_index_o]->time_base);
                packet->stream_index = video_index_o;
                if (av_interleaved_write_frame(ofmt_ctx, packet) < 0)
                {
                    av_packet_free(&packet);
                    break;
                }
            }
            av_packet_free(&packet);
        }
        else
        {
            packet = av_packet_alloc();
            if (av_read_frame(ifmt_ctx_a, packet) < 0)
            {
                av_packet_free(&packet);
                break;
            }
            if (packet->stream_index == audio_index)
            {
                cur_pts_a = packet->pts;
                packet->pts = av_rescale_q_rnd(packet->pts, ifmt_ctx_a->streams[video_index]->time_base, ofmt_ctx->streams[audio_index_o]->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                packet->dts = av_rescale_q_rnd(packet->dts, ifmt_ctx_a->streams[video_index]->time_base, ofmt_ctx->streams[audio_index_o]->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
                packet->duration = av_rescale_q(packet->duration, ifmt_ctx_a->streams[video_index]->time_base, ofmt_ctx->streams[audio_index_o]->time_base);
                packet->stream_index = audio_index_o;
                if (av_interleaved_write_frame(ofmt_ctx, packet) < 0)
                {
                    av_packet_free(&packet);
                    break;
                }
            }
            av_packet_free(&packet);
        }
    }
    av_write_trailer(ofmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        if (avio_closep(&ofmt_ctx->pb) < 0)
            return;
    avformat_close_input(&ifmt_ctx_v);
    ifmt_ctx_v = nullptr;
    avformat_close_input(&ifmt_ctx_a);
    ifmt_ctx_a = nullptr;
    avformat_free_context(ofmt_ctx);
    ofmt_ctx = nullptr;
//    QFile file(video_filename);
//    file.remove();
//    file.setFileName(audio_filename);
//    file.remove();
    emit save_finished();
}

void Recorder::start()
{
    avdevice_register_all();
    r.fps = {30, 1};
    QString tmp = QString::number(av_gettime());
    video_filename = tmp + "1.mp4";
    audio_filename = tmp + "1.aac";
    filename = "video/" + tmp + ".mp4";
    QDir dir("video");
    if (!dir.exists())
        if (!dir.mkpath(dir.absolutePath()))
            return;
    if (mode == 1)
    {
        if (initDXGIVideoInput())
        {
            FREE_VIDEO_DXGI_IN
            return;
        }
    }
    else
    {
        if (initGDIVideoInput())
        {
            FREE_VIDEO_GDI_IN
            return;
        }
    }
    r.ofmt_ctx_v = nullptr;
    if (avformat_alloc_output_context2(&r.ofmt_ctx_v, nullptr, nullptr, video_filename.toStdString().c_str()) < 0)
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        return;
    }
    if (initVideoOutput())
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        return;
    }
//    if (initVideoFilter()){
//        FREE_VIDEO_IN
//        FREE_VIDEO_OUT
//        FREE_VIDEO_FILTER
//        return;
//    }
    if (!(r.ofmt_ctx_v->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&(r.ofmt_ctx_v->pb), video_filename.toStdString().c_str(), AVIO_FLAG_READ_WRITE) < 0)
        {
            if (mode == 1)
            {
                FREE_VIDEO_DXGI_IN
            }
            else
            {
                FREE_VIDEO_GDI_IN
            }
            FREE_VIDEO_OUT
            return;
        }
    }
    if (avformat_write_header(r.ofmt_ctx_v, nullptr) < 0)
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        return;
    }
    CoInitialize(nullptr);
    if (initMicInput())
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        CoUninitialize();
        return;
    }
    if (initSpkRender())
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        CoUninitialize();
        return;
    }
    if (initSpkInput())
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        CoUninitialize();
        return;
    }
    r.ofmt_ctx_a = nullptr;
    if (avformat_alloc_output_context2(&r.ofmt_ctx_a, nullptr, nullptr, audio_filename.toStdString().c_str()) < 0)
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        CoUninitialize();
        return;
    }
    if (initAudioOutput())
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        FREE_AUDIO_OUT
        CoUninitialize();
        return;
    }
    if (initAudioFilter()){
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        FREE_AUDIO_OUT
        FREE_AUDIO_FILTER
        CoUninitialize();
        return;
    }
    if (!(r.ofmt_ctx_a->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&(r.ofmt_ctx_a->pb), audio_filename.toStdString().c_str(), AVIO_FLAG_READ_WRITE) < 0)
        {
            if (mode == 1)
            {
                FREE_VIDEO_DXGI_IN
            }
            else
            {
                FREE_VIDEO_GDI_IN
            }
            FREE_VIDEO_OUT
            FREE_MIC_IN
            FREE_SPK_RENDER
            FREE_SPK_IN
            FREE_AUDIO_OUT
            FREE_AUDIO_FILTER
            CoUninitialize();
            return;
        }
    }
    if (avformat_write_header(r.ofmt_ctx_a, nullptr) < 0)
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        FREE_AUDIO_OUT
        FREE_AUDIO_FILTER
        CoUninitialize();
        return;
    }
    record();
}

void Recorder::stop()
{
    r.state = RecordState::Finished;
}

void Recorder::finished()
{
    static int i = 0;
    if (++i < 3)
        return;
    render_thread->quit();
    render_thread->wait();
    mix_thread->quit();
    mix_thread->wait();
    video_thread->quit();
    video_thread->wait();
    write_thread->quit();
    write_thread->wait();
    while (render_thread->isRunning() && video_thread->isRunning() && mix_thread->isRunning() && write_thread->isRunning());
    delete render_thread;
    render_thread = nullptr;
    delete mix_thread;
    mix_thread = nullptr;
    delete video_thread;
    video_thread = nullptr;
    delete write_thread;
    write_thread = nullptr;
    delete render_spk;
    render_spk = nullptr;
    delete mix_audio;
    mix_audio = nullptr;
    delete record_video;
    record_video = nullptr;
    delete write_video;
    write_video = nullptr;

    if (av_write_trailer(r.ofmt_ctx_v))
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        FREE_AUDIO_OUT
        FREE_AUDIO_FILTER
        CoUninitialize();
        return;
    }
    if (r.ofmt_ctx_v && !(r.ofmt_ctx_v->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_closep(&r.ofmt_ctx_v->pb) < 0)
        {
            if (mode == 1)
            {
                FREE_VIDEO_DXGI_IN
            }
            else
            {
                FREE_VIDEO_GDI_IN
            }
            FREE_VIDEO_OUT
            FREE_MIC_IN
            FREE_SPK_RENDER
            FREE_SPK_IN
            FREE_AUDIO_OUT
            FREE_AUDIO_FILTER
            CoUninitialize();
            return;
        }
    }

    if (av_write_trailer(r.ofmt_ctx_a))
    {
        if (mode == 1)
        {
            FREE_VIDEO_DXGI_IN
        }
        else
        {
            FREE_VIDEO_GDI_IN
        }
        FREE_VIDEO_OUT
        FREE_MIC_IN
        FREE_SPK_RENDER
        FREE_SPK_IN
        FREE_AUDIO_OUT
        FREE_AUDIO_FILTER
        CoUninitialize();
        return;
    }
    if (r.ofmt_ctx_a && !(r.ofmt_ctx_a->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_closep(&r.ofmt_ctx_a->pb) < 0)
        {
            if (mode == 1)
            {
                FREE_VIDEO_DXGI_IN
            }
            else
            {
                FREE_VIDEO_GDI_IN
            }
            FREE_VIDEO_OUT
            FREE_MIC_IN
            FREE_SPK_RENDER
            FREE_SPK_IN
            FREE_AUDIO_OUT
            FREE_AUDIO_FILTER
            CoUninitialize();
            return;
        }
    }
    if (mode == 1)
    {
        FREE_VIDEO_DXGI_IN
    }
    else
    {
        FREE_VIDEO_GDI_IN
    }
    FREE_VIDEO_OUT
    FREE_MIC_IN
    FREE_SPK_RENDER
    FREE_SPK_IN
    FREE_AUDIO_OUT
    FREE_AUDIO_FILTER
    CoUninitialize();
    i = 0;
    save();
}
