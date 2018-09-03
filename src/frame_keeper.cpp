
#include "frame_keeper.hpp"
#include <iostream>
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
    while (!waiter.flag) {
       std::cv_status status = waiter.cond.wait_for(lck, std::chrono::seconds(1));
       if (std::cv_status::timeout == status) {
           break;
       }
    }
}

void frame_keeper::assig_new_frame(AVFrame* new_frame)
{
    std::lock_guard<std::mutex> lock(mtx);
    {
        std::lock_guard<std::mutex> lock(frame_mtx);
        if (frame != nullptr) {
            av_frame_free(&frame);
        }
        frame = av_frame_clone(new_frame);
    }
    for (auto el : sync_array) {
        el->flag.exchange(true);
        el->cond.notify_one();
    }
    sync_array.clear();
}

AVFrame* frame_keeper::get_frame()
{
    wait_new_frame();
    return get_current_frame();
}

AVFrame *frame_keeper::get_current_frame(){
    std::lock_guard<std::mutex> lock(frame_mtx);
    if (nullptr != frame) {
        return av_frame_clone(frame);
    }
    return nullptr;
}

void frame_keeper::setup_time_base(AVRational &base) {
    time_base = base;
}

AVRational frame_keeper::get_time_base(){
    return time_base;
}
