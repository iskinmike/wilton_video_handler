/*
 * Copyright 2018, myasnikov.mike at gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CHECKER_HPP
#define CHECKER_HPP

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdio.h>
#include <string>
#include "frame_keeper.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <set>
#include <chrono>

#include <asio.hpp>


using send_handler = std::function<void(const asio::error_code&, std::size_t)>;

class client_session
{
	asio::ip::tcp::socket socket;
public:
    client_session(asio::io_service& io_service);
    ~client_session();

    asio::ip::tcp::socket& get_socket();
    void send(std::string message, send_handler handler);
    void close();
    bool is_open();
};

class alert_server
{
    asio::io_service io_service;
    asio::ip::tcp::endpoint endpoint;
    asio::ip::tcp::acceptor acceptor;
    std::set<std::shared_ptr<client_session>> sessions;

    asio::error_code ec;
    int alert_number;
    std::thread service_thread;

public:
    alert_server(unsigned short port = 7777, std::string ip = "127.0.0.1");
    ~alert_server();

    bool listen();
    void start_accept();
    void stop();
    void send_cheating_alert_message(int faces_count);
    void send_error_message(std::string error);
    void drop_alerts();
};

class checker
{
    AVFrame* frame;
    cv::Mat frame_mat;

    std::atomic_bool stop_flag;
    std::thread checker_display_thread;
    cv::CascadeClassifier face_cascade;
//    cv::CascadeClassifier eyes_cascade;
    cv::String face_cascade_path;
//    cv::String eyes_cascade_name;

    alert_server alert_service;

    bool checking_in_progress;
    const int allowed_faces_count = 1;
    int wait_time_ms;
public:
    checker(std::string ip, int port, std::string face_cascade_path, int64_t wait_time_ms);
    ~checker();

    void convert_frame_to_mat();
    int detect_faces();
    int alert_cheating(int faces_count);
    void check_faces();
    void display_mat();

    std::string run_checker_display();
    bool is_checking(){
        return checking_in_progress;
    }
    void stop_cheking_display();

    void init();
};


#endif  /* CHECKER_HPP */
