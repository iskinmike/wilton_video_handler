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

#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/types_c.h"

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
#include <iostream>
#include <fstream>

using send_handler = std::function<void(const asio::error_code&, std::size_t)>;
//#define TEST_OUT 1

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

class template_detector
{
public:
#ifdef TEST_OUT
    static uint64_t next_id;
    uint64_t uniq_id;
#endif
    // template matching
    cv::Mat face_template;
    cv::Mat compare_template;
    cv::Mat origin_template;
    cv::Mat matching_result;

    cv::Rect face_area;
    cv::Rect tracked_face_area;
    cv::Point prev_point;

    bool is_face_detected;
    double percent;

    template_detector(const cv::Mat& image, const cv::Rect& face);

    void set_prev_point(const cv::Point& point);
    void drop_prev_point();
    void update_template(const cv::Mat &frame, cv::Rect face);

    cv::Rect double_size(cv::Rect view, cv::Rect border_rect);
    cv::Rect match(cv::Mat frame);

    double calc_vector(const cv::Point& point);
};

class matcher
{
    std::set<std::shared_ptr<template_detector>> templates;
    std::set<std::shared_ptr<template_detector>> new_templates;
public:
    matcher(){}
    ~matcher(){}

    void update_templates(std::vector<cv::Rect>& areas, const cv::Mat& image);
    void search_faces(std::vector<cv::Rect>& faces, const cv::Mat &image);
    void remove_intersected_templates();

    int get_found_faces_count();
};

class recognizer
{
    AVFrame* frame;
    cv::Mat frame_mat;

    std::atomic_bool stop_flag;
    std::thread recognizer_display_thread;
    cv::CascadeClassifier face_cascade;
    cv::String face_cascade_path;
    alert_server alert_service;

    bool recognizing_in_progress;
    int wait_time_ms;
    const int allowed_faces_count = 1;

public:
    int prev_finded_faces;
    matcher face_matcher;
    recognizer(std::string ip, int port, std::string face_cascade_path, int64_t wait_time_ms);
    ~recognizer();

    void convert_frame_to_mat();
    int detect_faces();
    int alert_cheating(int faces_count);
    void recognize_faces();
    void display_mat();

    std::string run_recognizer_display();
    bool is_recognizing();
    void stop_cheking_display();

    void init();
};


#endif  /* RECOGNIZER_HPP */
