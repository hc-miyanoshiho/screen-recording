#include "renderspk.h"
#include <QDebug>

using namespace recorder;

RenderSpk::RenderSpk(record_t &r, QObject *parent) : QObject(parent), r(r)
{

}

void RenderSpk::render()
{
    HRESULT hr;
    BYTE *data = nullptr;
    UINT32 frames_of_padding = 0;
    UINT need_data_len = 0;
    qDebug() << "render sleep time = " << r.render_sleep_time;
    while (r.state == RecordState::Recording)
    {
        Sleep(r.render_sleep_time);
        hr = r.audio_client_render->GetCurrentPadding(&frames_of_padding);
        if (FAILED(hr))
        {
            r.state = RecordState::Finished;
            break;
        }
        if (r.frame_buffer_size == frames_of_padding)
            continue;
        need_data_len = r.frame_buffer_size - frames_of_padding;
        hr = r.audio_render_client->GetBuffer(need_data_len, &data);
        if (FAILED(hr))
        {
            r.state = RecordState::Finished;
            break;
        }
        r.audio_render_client->ReleaseBuffer(need_data_len, 0);
    }
    qDebug() << "render over...";
    emit over();
}
