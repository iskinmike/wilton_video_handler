
#include "frame_keeper.h"

FrameKeeper::~FrameKeeper(){
    av_frame_free(&frame);
}

void FrameKeeper::assigNewFrame(AVFrame* new_frame)
{
    std::lock_guard<std::mutex> lock(mtx);
    av_frame_free(&frame);
    frame = av_frame_clone(new_frame);
    for (auto el : sync_array) {
        el->flag.exchange(true);
        el->cond.notify_all();
    }
    sync_array.clear();
}

AVFrame *FrameKeeper::getFrame()
{
    SyncWaiter waiter;
    waiter.flag.exchange(false);
    {
        std::lock_guard<std::mutex> lock(mtx);
        sync_array.push_back(&waiter);
    }
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!waiter.flag) waiter.cond.wait_for(lck, std::chrono::seconds(1));

    return av_frame_clone(frame); // Пока попробуем так чтобы ничего не терять и не освободить данные случайно
}
