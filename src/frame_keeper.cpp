
#include "frame_keeper.hpp"

FrameKeeper::~FrameKeeper(){
    av_frame_free(&frame);
}

void FrameKeeper::waitNewFrame()
{
    SyncWaiter waiter;
    waiter.flag.exchange(false);
    {
        std::lock_guard<std::mutex> lock(mtx);
        sync_array.push_back(&waiter);
    }
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!waiter.flag) waiter.cond.wait_for(lck, std::chrono::seconds(1));
}

void FrameKeeper::assigNewFrames(AVFrame* new_frame, AVFrame *new_origin_frame)
{
    std::lock_guard<std::mutex> lock(mtx);
    av_frame_free(&frame);
    av_frame_free(&origin_frame);
    frame = av_frame_clone(new_frame);
    origin_frame = av_frame_clone(new_origin_frame);
    for (auto el : sync_array) {
        el->flag.exchange(true);
        el->cond.notify_all();
    }
    sync_array.clear();
}

AVFrame* FrameKeeper::getFrame()
{
    waitNewFrame();
    return av_frame_clone(frame);
}

AVFrame* FrameKeeper::getOriginFrame()
{
    waitNewFrame();
    return av_frame_clone(origin_frame);
}


