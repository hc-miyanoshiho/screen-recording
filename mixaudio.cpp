#include "mixaudio.h"
#include <QDebug>

using namespace recorder;

MixAudio::MixAudio(record_t &r, QObject *parent) : QObject(parent), r(r)
{

}

void MixAudio::mix()
{
    UINT32 next_packet_size = 0;
    BYTE *data = nullptr;
    UINT32 num_frames_to_read = 0;
    DWORD flags = 0;
    HRESULT hr;

    int64_t pts_mic = 0, pts_sys = 0;
    int64_t pts = 0;
    uint8_t **audio_dst_data_buffer = nullptr;
    qDebug() << "mic sleep time = " << r.mic_sleep_time;
    qDebug() << "spk sleep time = " << r.spk_sleep_time;
    qint64 min_sleep_time = r.mic_sleep_time < r.spk_sleep_time ? r.mic_sleep_time : r.spk_sleep_time;
    qint64 t = QDateTime::currentMSecsSinceEpoch();
    qint64 sleep_time;
    while (r.state == RecordState::Recording)
    {
        sleep_time = min_sleep_time + t - QDateTime::currentMSecsSinceEpoch();
        if (sleep_time > 0)
            Sleep(sleep_time);
//        Sleep(r.mic_sleep_time);
        while (av_audio_fifo_size(r.audio_fifo_mic) < r.ocode_ctx_a->frame_size && r.state == RecordState::Recording)
        {
            hr = r.audio_capture_client_mic->GetBuffer(&data, &num_frames_to_read, &flags, nullptr, nullptr);
            if (FAILED(hr))
            {
                r.state = RecordState::Finished;
                emit write_finished();
                return;
            }
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                data = nullptr;
            if (num_frames_to_read)
            {
                frame = av_frame_alloc();
                frame->sample_rate = r.spk_format->nSamplesPerSec;
                frame->channels = r.spk_format->nChannels;
                frame->nb_samples = num_frames_to_read;
                frame->format = AV_SAMPLE_FMT_S16;
                av_samples_fill_arrays(frame->data, frame->linesize, data, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
                av_samples_alloc_array_and_samples(&audio_dst_data_buffer, nullptr, r.ocode_ctx_a->channels, frame->nb_samples, r.ocode_ctx_a->sample_fmt, 1);
                swr_convert(r.swr_ctx_mic, audio_dst_data_buffer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                if(av_audio_fifo_realloc(r.audio_fifo_mic, av_audio_fifo_size(r.audio_fifo_mic) + frame->nb_samples) < 0)
                {
                    r.state = RecordState::Finished;
                    emit write_finished();
                    return;
                }
                av_audio_fifo_write(r.audio_fifo_mic, (void**)audio_dst_data_buffer, frame->nb_samples);
                if (audio_dst_data_buffer)
                {
                    av_free(audio_dst_data_buffer[0]);
                    av_free(audio_dst_data_buffer);
                }
                if (swr_get_out_samples(r.swr_ctx_mic, 0) >= frame->nb_samples)
                {
                    av_samples_alloc_array_and_samples(&audio_dst_data_buffer, nullptr, r.ocode_ctx_a->channels, frame->nb_samples, r.ocode_ctx_a->sample_fmt, 1);
                    swr_convert(r.swr_ctx_mic, audio_dst_data_buffer, frame->nb_samples, nullptr, 0);
                    if (av_audio_fifo_realloc(r.audio_fifo_mic, av_audio_fifo_size(r.audio_fifo_mic) + frame->nb_samples) < 0)
                    {
                        r.state = RecordState::Finished;
                        emit write_finished();
                        return;
                    }
                    av_audio_fifo_write(r.audio_fifo_mic, (void**)audio_dst_data_buffer, frame->nb_samples);
                    if (audio_dst_data_buffer)
                    {
                        av_free(audio_dst_data_buffer[0]);
                        av_free(audio_dst_data_buffer);
                    }
                }
                av_frame_free(&frame);
            }
            r.audio_capture_client_mic->ReleaseBuffer(num_frames_to_read);
        }
//        Sleep(r.spk_sleep_time);
        while (av_audio_fifo_size(r.audio_fifo_spk) < r.ocode_ctx_a->frame_size && r.state == RecordState::Recording)
        {
            hr = r.audio_capture_client_spk->GetNextPacketSize(&next_packet_size);
            if (FAILED(hr))
            {
                r.state = RecordState::Finished;
                emit write_finished();
                return;
            }
            if (next_packet_size == 0)
                continue;
            hr = r.audio_capture_client_spk->GetBuffer(&data, &num_frames_to_read, &flags, nullptr, nullptr);
            if (FAILED(hr))
            {
                r.state = RecordState::Finished;
                emit write_finished();
                return;
            }
//            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
//                ;
            if (num_frames_to_read)
            {
                frame = av_frame_alloc();
                frame->sample_rate = r.spk_format->nSamplesPerSec;
                frame->channels = r.spk_format->nChannels;
                frame->nb_samples = num_frames_to_read;
                frame->format = AV_SAMPLE_FMT_S16;
                av_samples_fill_arrays(frame->data, frame->linesize, data, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
                av_samples_alloc_array_and_samples(&audio_dst_data_buffer, nullptr, r.ocode_ctx_a->channels, frame->nb_samples, r.ocode_ctx_a->sample_fmt, 1);
                swr_convert(r.swr_ctx_spk, audio_dst_data_buffer, r.ocode_ctx_a->frame_size, (const uint8_t**)frame->data, frame->nb_samples);
                if (av_audio_fifo_realloc(r.audio_fifo_spk, av_audio_fifo_size(r.audio_fifo_spk) + frame->nb_samples) < 0)
                {
                    r.state = RecordState::Finished;
                    emit write_finished();
                    return;
                }
                av_audio_fifo_write(r.audio_fifo_spk, (void**)audio_dst_data_buffer, frame->nb_samples);
                if (audio_dst_data_buffer)
                {
                    av_free(audio_dst_data_buffer[0]);
                    av_free(audio_dst_data_buffer);
                }
                if (swr_get_out_samples(r.swr_ctx_spk, 0) >= frame->nb_samples)
                {
                    av_samples_alloc_array_and_samples(&audio_dst_data_buffer, nullptr, r.ocode_ctx_a->channels, frame->nb_samples, r.ocode_ctx_a->sample_fmt, 1);
                    swr_convert(r.swr_ctx_spk, audio_dst_data_buffer, frame->nb_samples, nullptr, 0);
                    if (av_audio_fifo_realloc(r.audio_fifo_spk, av_audio_fifo_size(r.audio_fifo_spk) + frame->nb_samples) < 0)
                    {
                        r.state = RecordState::Finished;
                        emit write_finished();
                        return;
                    }
                    av_audio_fifo_write(r.audio_fifo_spk, (void**)audio_dst_data_buffer, frame->nb_samples);
                    if (audio_dst_data_buffer)
                    {
                        av_free(audio_dst_data_buffer[0]);
                        av_free(audio_dst_data_buffer);
                    }
                }
                av_frame_free(&frame);
            }
            r.audio_capture_client_spk->ReleaseBuffer(num_frames_to_read);
        }
        t = QDateTime::currentMSecsSinceEpoch();
        while (av_audio_fifo_size(r.audio_fifo_mic) >= r.ocode_ctx_a->frame_size && av_audio_fifo_size(r.audio_fifo_spk) >= r.ocode_ctx_a->frame_size)
        {
            frameAAC = av_frame_alloc();
            frameAAC->format = r.ocode_ctx_a->sample_fmt;
            frameAAC->sample_rate = r.ocode_ctx_a->sample_rate;
            frameAAC->nb_samples = r.ocode_ctx_a->frame_size;
            frameAAC->channel_layout = r.ocode_ctx_a->channel_layout;
            av_frame_get_buffer(frameAAC, 0);
            frameAAC->pts = pts_mic;
            pts_mic += r.ocode_ctx_a->frame_size;
            av_audio_fifo_read(r.audio_fifo_mic, (void**)frameAAC->data, r.ocode_ctx_a->frame_size);
            if (av_buffersrc_add_frame(r.abuffer_ctx1, frameAAC) < 0)
            {
                av_frame_free(&frameAAC);
                {
                    r.state = RecordState::Finished;
                    emit write_finished();
                    return;
                }
            }
            av_frame_free(&frameAAC);

            frameAAC = av_frame_alloc();
            frameAAC->format = r.ocode_ctx_a->sample_fmt;
            frameAAC->sample_rate = r.ocode_ctx_a->sample_rate;
            frameAAC->nb_samples = r.ocode_ctx_a->frame_size;
            frameAAC->channel_layout = r.ocode_ctx_a->channel_layout;
            av_frame_get_buffer(frameAAC, 0);
            frameAAC->pts = pts_sys;
            pts_sys += r.ocode_ctx_a->frame_size;
            av_audio_fifo_read(r.audio_fifo_spk, (void**)frameAAC->data, r.ocode_ctx_a->frame_size);
            if (av_buffersrc_add_frame(r.abuffer_ctx2, frameAAC) < 0)
            {
                av_frame_free(&frameAAC);
                {
                    r.state = RecordState::Finished;
                    emit write_finished();
                    return;
                }
            }
            av_frame_free(&frameAAC);

            frameAAC = av_frame_alloc();
            if (av_buffersink_get_frame(r.sink_ctx, frameAAC) < 0)
            {
                av_frame_free(&frameAAC);
                continue;
            }
            if (avcodec_send_frame(r.ocode_ctx_a, frameAAC) < 0)
            {
                av_frame_free(&frameAAC);
                continue;
            }
            packetAAC = av_packet_alloc();
            if (avcodec_receive_packet(r.ocode_ctx_a, packetAAC) < 0)
            {
                av_frame_free(&frameAAC);
                av_packet_free(&packetAAC);
                continue;
            }
            packetAAC->stream_index = r.audioIndex_o;
            packetAAC->pts = pts;
            packetAAC->dts = pts;
            pts += r.ocode_ctx_a->frame_size;
            packetAAC->pts = av_rescale_q_rnd(packetAAC->pts, r.ofmt_ctx_a->streams[r.audioIndex_o]->time_base, r.a_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packetAAC->dts = av_rescale_q_rnd(packetAAC->dts, r.ofmt_ctx_a->streams[r.audioIndex_o]->time_base, r.a_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            packetAAC->duration = av_rescale_q(packetAAC->duration, r.ofmt_ctx_a->streams[r.audioIndex_o]->time_base, r.a_stream->time_base);
            if (av_interleaved_write_frame(r.ofmt_ctx_a, packetAAC) < 0)
            {
                av_frame_free(&frameAAC);
                av_packet_free(&packetAAC);
                return;
            }
            av_frame_free(&frameAAC);
            av_packet_free(&packetAAC);
        }
    }
    qDebug() << "mix over...";
    emit write_finished();
}
