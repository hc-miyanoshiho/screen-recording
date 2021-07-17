#include "writevideo.h"
#include <QDebug>

using namespace recorder;

WriteVideo::WriteVideo(record_t &r, queue_t &q, int mode, QObject *parent) : QObject(parent), r(r), q(q), mode(mode)
{

}

//void WriteVideo::write()
//{
//    int64_t index = 0;
//    uint8_t *outbuffer = nullptr;
//    while (1)
//    {
//        r.mutex.lock();
//        if (r.state != RecordState::Recording && (q.video_frames.size() == 0))
//        {
//            r.mutex.unlock();
//            break;
//        }
//        if (q.video_frames.size())
//        {
//            outbuffer = (uint8_t*)av_malloc(r.size);
//            AVFrame *frame = q.video_frames.front();
//            AVFrame *frameYUV = av_frame_alloc();
//            av_image_fill_arrays(frameYUV->data, frameYUV->linesize, outbuffer, AV_PIX_FMT_YUV420P, r.ocode_ctx_v->width, r.ocode_ctx_v->height, 1);
//            frameYUV->format = AV_PIX_FMT_YUV420P;
//            frameYUV->width = r.ocode_ctx_v->width;
//            frameYUV->height = r.ocode_ctx_v->height;
//            frameYUV->pts = index;
//            sws_scale(r.sws_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, r.desc.Height, frameYUV->data, frameYUV->linesize);
//            q.video_frames.pop();
//            r.mutex.unlock();
//            av_frame_free(&frame);
//            if (avcodec_send_frame(r.ocode_ctx_v, frameYUV) < 0)
//            {
//                av_frame_free(&frameYUV);
//                av_free(outbuffer);
//                outbuffer = nullptr;
//                continue;
//            }
//            packetYUV = av_packet_alloc();
//            if (avcodec_receive_packet(r.ocode_ctx_v, packetYUV) < 0)
//            {
//                av_packet_free(&packetYUV);
//                av_frame_free(&frameYUV);
//                av_free(outbuffer);
//                outbuffer = nullptr;
//                continue;
//            }
//            packetYUV->stream_index = r.videoIndex_o;
//            packetYUV->pts = index++;
//            packetYUV->dts = packetYUV->pts;
//            packetYUV->duration = 1;    //也能够音画同步。。并且视频时间总长正确
////            packetYUV->duration = r.v_stream->time_base.den * r.fps.den / r.v_stream->time_base.num / r.fps.num;  //能够正确同步音画，视频总长错误
//            av_packet_rescale_ts(packetYUV, r.ocode_ctx_v->time_base, r.v_stream->time_base);
//            packetYUV->pts = av_rescale_q_rnd(packetYUV->pts, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//            packetYUV->dts = av_rescale_q_rnd(packetYUV->dts, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//            packetYUV->duration = av_rescale_q(packetYUV->duration, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base);
//            if (av_interleaved_write_frame(r.ofmt_ctx_v, packetYUV) < 0)
//            {
//                emit write_finished();
//                return;
//            }
//            av_frame_free(&frameYUV);
//            av_packet_free(&packetYUV);
//            av_free(outbuffer);
//            outbuffer = nullptr;
//        }
//        else
//        {
//            r.mutex.unlock();
//            Sleep(1);
//        }
//    }
//    avcodec_send_frame(r.ocode_ctx_v, NULL);
//    while (1)
//    {
//        AVPacket *_packetYUV = av_packet_alloc();
//        if (avcodec_receive_packet(r.ocode_ctx_v, _packetYUV) == AVERROR_EOF)
//        {
//            av_packet_free(&_packetYUV);
//            break;
//        }
//        av_packet_free(&_packetYUV);
//    }
//    while (q.video_frames.size())
//    {
//        AVFrame *frame = q.video_frames.front();
//        av_frame_free(&frame);
//        q.video_frames.pop();
//    }
//    qDebug() << "video over...";
//    emit write_finished();
//}

void WriteVideo::write()
{
    int64_t index = 0;
    uint8_t *outbuffer = nullptr;
    int h = 0;
    if (mode == 1)
        h = r.desc.Height;
    else
        h = r.icode_ctx_v->height;
    while (1)
    {
        r.mutex.lock();
        if (r.state != RecordState::Recording && (q.video_frames.size() == 0))
        {
            r.mutex.unlock();
            break;
        }
        if (q.video_frames.size())
        {
            outbuffer = (uint8_t*)av_malloc(r.size);
            frame = q.video_frames.front();
            frameYUV = av_frame_alloc();
            av_image_fill_arrays(frameYUV->data, frameYUV->linesize, outbuffer, AV_PIX_FMT_YUV420P, r.ocode_ctx_v->width, r.ocode_ctx_v->height, 1);
            frameYUV->format = AV_PIX_FMT_YUV420P;
            frameYUV->width = r.ocode_ctx_v->width;
            frameYUV->height = r.ocode_ctx_v->height;
            frameYUV->pts = index;
            sws_scale(r.sws_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, h, frameYUV->data, frameYUV->linesize);
            q.video_frames.pop();
            r.mutex.unlock();
            av_frame_free(&frame);
            if (avcodec_send_frame(r.ocode_ctx_v, frameYUV) < 0)
            {
                av_frame_free(&frameYUV);
                av_free(outbuffer);
                outbuffer = nullptr;
                continue;
            }
            packetYUV = av_packet_alloc();
            if (avcodec_receive_packet(r.ocode_ctx_v, packetYUV) < 0)
            {
                av_packet_free(&packetYUV);
                av_frame_free(&frameYUV);
                av_free(outbuffer);
                outbuffer = nullptr;
                continue;
            }
            packetYUV->stream_index = r.videoIndex_o;
            packetYUV->pts = index++;
            packetYUV->dts = packetYUV->pts;
            packetYUV->duration = 1;    //能够音画同步。。且总时长正确
//            packetYUV->duration = r.v_stream->time_base.den * r.fps.den / r.v_stream->time_base.num / r.fps.num;  //能够正确同步音画。。但总时长不准确
            av_packet_rescale_ts(packetYUV, r.ocode_ctx_v->time_base, r.v_stream->time_base);
            packetYUV->pts = av_rescale_q_rnd(packetYUV->pts, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packetYUV->dts = av_rescale_q_rnd(packetYUV->dts, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packetYUV->duration = av_rescale_q(packetYUV->duration, r.ofmt_ctx_v->streams[r.videoIndex_o]->time_base, r.v_stream->time_base);
            if (av_interleaved_write_frame(r.ofmt_ctx_v, packetYUV) < 0)
            {
                emit write_finished();
                return;
            }
            av_frame_free(&frameYUV);
            av_packet_free(&packetYUV);
            av_free(outbuffer);
            outbuffer = nullptr;
        }
        else
        {
            r.mutex.unlock();
            Sleep(1);
        }
    }
    avcodec_send_frame(r.ocode_ctx_v, NULL);
    while (1)
    {
        packetYUV = av_packet_alloc();
        if (avcodec_receive_packet(r.ocode_ctx_v, packetYUV) == AVERROR_EOF)
        {
            av_packet_free(&packetYUV);
            break;
        }
        av_packet_free(&packetYUV);
    }
    while (q.video_frames.size())
    {
        frame = q.video_frames.front();
        q.video_frames.pop();
        av_frame_free(&frame);
    }
    qDebug() << "write over";
    emit write_finished();
}
