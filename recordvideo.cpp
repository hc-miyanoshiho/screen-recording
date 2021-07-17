#include "recordvideo.h"
#include <QImage>
#include <QDebug>

using namespace recorder;

RecordVideo::RecordVideo(record_t &r, queue_t &q, QObject *parent) : QObject(parent), r(r), q(q)
{

}

void RecordVideo::record_gdi()
{
    int64_t index = 0;
    frame_tmp = nullptr;
    int64_t t = av_gettime();
    AVPacket *packet = nullptr;
    while (r.state == RecordState::Recording)
    {
        if (av_gettime() - t < index * r.fps.den * 1000000 / r.fps.num)
        {
            Sleep(1);
            continue;
        }
        if (frame_tmp && av_gettime() - t >= (index + 1) * r.fps.den * 1000000 / r.fps.num)
        {
            frame = av_frame_clone(frame_tmp);
            index++;
//            if (av_buffersrc_add_frame_flags(r.buffersrc_ctx, _frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
//                break;
//            AVFrame *filt_frame = av_frame_alloc();
//            if (av_buffersink_get_frame(r.buffersink_ctx, filt_frame) < 0)
//                break;
//            av_frame_free(&_frame);
            r.mutex.lock();
//            q.video_frames.push(filt_frame);
            q.video_frames.push(frame);
            r.mutex.unlock();
            Sleep(1);
            continue;
        }
        packet = av_packet_alloc();
        if (av_read_frame(r.ifmt_ctx_v, packet) < 0)
        {
            av_packet_free(&packet);
            break;
        }
        if (avcodec_send_packet(r.icode_ctx_v, packet) < 0)
        {
            av_packet_free(&packet);
            continue;;
        }
        frame = av_frame_alloc();
        if (avcodec_receive_frame(r.icode_ctx_v, frame) < 0)
        {
            av_packet_free(&packet);
            av_frame_free(&frame);
            continue;
        }
        av_packet_free(&packet);
        if (frame_tmp)
        {
            av_frame_free(&frame_tmp);
            frame_tmp = nullptr;
        }
        frame_tmp = av_frame_clone(frame);
//        if (av_buffersrc_add_frame_flags(r.buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
//            break;
//        AVFrame *filt_frame = av_frame_alloc();
//        if (av_buffersink_get_frame(r.buffersink_ctx, filt_frame) < 0)
//            break;
//        av_frame_free(&frame);
        index++;
        r.mutex.lock();
//        q.video_frames.push(filt_frame);
        q.video_frames.push(frame);
        r.mutex.unlock();
    }
    qDebug() << "gdi video over";
}

void RecordVideo::record_dxgi()
{
    int64_t index = 0;
    frame_tmp = nullptr;
    int real_row_pitch = 0;
    int real_size = 0;
    uchar *dd = nullptr;
    ID3D11Texture2D *dxgi_text2d = nullptr;
    IDXGISurface *dxgi_surf = nullptr;
    DXGI_MAPPED_RECT mappedRect;
    int64_t t = av_gettime();
    while (r.state == RecordState::Recording)
    {
        if (av_gettime() - t < index * r.fps.den * 1000000 / r.fps.num)
        {
            Sleep(1);
            continue;
        }
        r.hr = r.dxgi_output_dupl->AcquireNextFrame(0, &r.frame_info, &r.dxgi_res);
        if (r.hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            if (frame_tmp && av_gettime() - t >= (index + 1) * r.fps.den * 1000000 / r.fps.num)
            {
                frame = av_frame_clone(frame_tmp);
                if (!frame)
                {
                    r.state = RecordState::Finished;
                    break;
                }
                frame->pts = index++;
//                if(av_buffersrc_add_frame_flags(r.buffersrc_ctx, _frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
//                    break;
//                AVFrame *filt_frame = av_frame_alloc();
//                if(av_buffersink_get_frame(r.buffersink_ctx, filt_frame) < 0)
//                    break;
//                av_frame_free(&_frame);
                r.mutex.lock();
//                q.video_frames.push(filt_frame);
                q.video_frames.push(frame);
                r.mutex.unlock();
            }
            Sleep(1);
            continue;
        }
        else if (FAILED(r.hr))
        {
            r.state = RecordState::Finished;
            break;
        }
        if (r.no_mouse_image)
        {
            r.no_mouse_image->Release();
            r.no_mouse_image = nullptr;
        }
        r.hr = r.dxgi_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&r.no_mouse_image));
        if (FAILED(r.hr))
        {
            r.state = RecordState::Finished;
            break;
        }
        r.dxgi_res->Release();
        r.dxgi_res = nullptr;
        r.d3d_device->CreateTexture2D(&r.desc, NULL, &dxgi_text2d);
        r.d3d_device_ctx->CopyResource(dxgi_text2d, r.no_mouse_image);
        dxgi_text2d->QueryInterface(__uuidof(IDXGISurface), (void**)&dxgi_surf);
        dxgi_surf->Map(&mappedRect, DXGI_MAP_READ);
//        r.d3d_device_ctx->CopyResource(r.add_mouse_image, r.no_mouse_image);
//        r.hr = r.add_mouse_image->QueryInterface(IID_PPV_ARGS(&r.dxgi_surface1));
//        if (FAILED(r.hr))
//            return;
//        r.cursor_info.cbSize = sizeof(r.cursor_info);
//        if (GetCursorInfo(&r.cursor_info))
//        {
//            if (r.cursor_info.flags == CURSOR_SHOWING)
//            {
//                r.dxgi_surface1->GetDC(FALSE, &r.hdc);
//                DrawIconEx(r.hdc, r.cursor_info.ptScreenPos.x, r.cursor_info.ptScreenPos.y, r.cursor_info.hCursor, 0, 0, 0, 0, DI_NORMAL | DI_DEFAULTSIZE);
//                r.dxgi_surface1->ReleaseDC(nullptr);
//                r.dxgi_surface1 = nullptr;
//            }
//        }
        real_row_pitch = (uint)mappedRect.Pitch < (r.desc.Width << 2) ? mappedRect.Pitch : (r.desc.Width << 2);
        real_size = real_row_pitch * r.desc.Height;
        if (dd)
        {
            delete []dd;
            dd = nullptr;
        }
        dd = new uchar[real_size];
        for (uint l = 0; l < r.desc.Height; l++)
            memcpy_s(dd + real_row_pitch * l, real_row_pitch, (uchar*)mappedRect.pBits + mappedRect.Pitch * l, real_row_pitch);
        dxgi_text2d->Release();
        dxgi_surf->Release();
        r.hr = r.dxgi_output_dupl->ReleaseFrame();
        if (FAILED(r.hr))
        {
            r.state = RecordState::Finished;
            break;
        }
        if (r.no_mouse_image)
        {
            r.no_mouse_image->Release();
            r.no_mouse_image = nullptr;
        }
        frame = av_frame_alloc();
        av_image_fill_arrays(frame->data, frame->linesize, dd, AV_PIX_FMT_BGRA, r.desc.Width, r.desc.Height, 1);
        frame->format = AV_PIX_FMT_BGRA;
        frame->width = r.desc.Width;
        frame->height = r.desc.Height;
        frame->pts = index++;
//        if(av_buffersrc_add_frame_flags(r.buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0){
//            break;
//        }
//        AVFrame *filt_frame = av_frame_alloc();
//        if(av_buffersink_get_frame(r.buffersink_ctx, filt_frame) < 0){
//            break;
//        }
//        av_frame_free(&frame);
        frame_ref = av_frame_clone(frame);
        if (!frame_ref)
        {
            r.state = RecordState::Finished;
            break;
        }
        av_frame_free(&frame);
        if (frame_tmp)
        {
            av_frame_free(&frame_tmp);
            frame_tmp = nullptr;
        }
        frame_tmp = av_frame_alloc();
        av_image_fill_arrays(frame_tmp->data, frame_tmp->linesize, dd, AV_PIX_FMT_BGRA, r.desc.Width, r.desc.Height, 1);
        frame_tmp->format = AV_PIX_FMT_BGRA;
        frame_tmp->width = r.desc.Width;
        frame_tmp->height = r.desc.Height;
        r.mutex.lock();
//        q.video_frames.push(filt_frame);
        q.video_frames.push(frame_ref);
        r.mutex.unlock();
    }
    if (dd)
    {
        delete []dd;
        dd = nullptr;
    }
    if (frame_tmp)
    {
        av_frame_free(&frame_tmp);
        frame_tmp = nullptr;
    }
    qDebug() << "dxgi video over";
}
