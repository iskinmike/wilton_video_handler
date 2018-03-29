
#include "frame_keeper.hpp"

frame_keeper::~frame_keeper(){
    av_frame_free(&frame);
}

void frame_keeper::wait_new_frame()
{
    sync_waiter waiter;
    waiter.flag.exchange(false);
    {
        std::lock_guard<std::mutex> lock(mtx);
        sync_array.push_back(&waiter);
    }
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!waiter.flag) waiter.cond.wait_for(lck, std::chrono::seconds(1));
}

void frame_keeper::assig_new_frames(AVFrame* new_frame, AVFrame *new_origin_frame)
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

AVFrame* frame_keeper::get_frame()
{
    wait_new_frame();
    return av_frame_clone(frame);
}

AVFrame* frame_keeper::get_origin_frame()
{
    wait_new_frame();
    return av_frame_clone(origin_frame);
}
