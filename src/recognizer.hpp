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
#include <sstream>
#include <ostream>

using send_handler = std::function<void(const asio::error_code&, std::size_t)>;

class storage_logger
{
protected:
    std::ostringstream logger_storage;
public:
    storage_logger(const storage_logger& other);
    storage_logger& operator=(const storage_logger& other);
    storage_logger() {}
    ~storage_logger() {}
    std::string get_logged() const;
    void drop_logger();
};

class recognize_logger
{
    std::string path;
public:
    std::fstream fs;
    explicit recognize_logger(const std::string& path) : path(path) {}
    void open();
};

class client_session
{
	asio::ip::tcp::socket socket;
public:
    explicit client_session(asio::io_service& io_service);
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
    alert_server(unsigned short port = 7777, const std::string& ip = "127.0.0.1");
    ~alert_server();

    bool listen();
    void start_accept();
    void stop();
    void send_cheating_alert_message(int faces_count);
    void send_error_message(std::string error);
    void drop_alerts();
};

struct template_detector_settings {
    // this value allows to detect face rotation from frontal to profile
    double min_threshold;
    int enable_logging;
};

class template_detector : public storage_logger
{

public:
    static uint64_t next_id;
    uint64_t uniq_id;
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
    template_detector_settings set;


    template_detector(const cv::Mat& image, const cv::Rect& face, template_detector_settings set);

    void set_prev_point(const cv::Point& point);
    void drop_prev_point();
    void update_template(const cv::Mat &frame, cv::Rect face);

    cv::Rect double_size(cv::Rect view, cv::Rect border_rect);
    cv::Rect match(cv::Mat frame);

    double calc_vector(const cv::Point& point);
};

struct matcher_settings {
    // this value determines intersection between new finded face and face search area of already created template
    double intersection_percent_threshold;
    int enable_logging;
    // for template detector
    template_detector_settings template_set;
};

class matcher  : public storage_logger
{
    std::set<std::shared_ptr<template_detector>> templates;
    std::set<std::shared_ptr<template_detector>> new_templates;
    matcher_settings set;
public:
    explicit matcher(const matcher_settings& set);
    ~matcher(){}

    void update_templates(std::vector<cv::Rect>& areas, const cv::Mat& image);
    void search_faces(std::vector<cv::Rect>& faces, const cv::Mat &image);
    void remove_intersected_templates();

    int get_found_faces_count();
};

struct recognizer_settings {
    std::string ip;
    int port;
    std::string face_cascade_path;
    int64_t wait_time_ms;
    int width;
    int height;
    // detect cascade options
    double scale;
    int near_faces;
    int min_size_x;
    int min_size_y;
    int max_size_x;
    int max_size_y;
    // for matcher
    matcher_settings matcher_set;
    // support options
    int enable_debug_display;
    int enable_logger;
    std::string logger_tmp_file;
};

class recognizer
{
    recognizer_settings set;
    cv::Mat frame_mat;

    std::atomic_bool stop_flag;
    std::thread recognizer_display_thread;
    cv::CascadeClassifier face_cascade;
    cv::String face_cascade_path;
    alert_server alert_service;
    std::string debug_window_name;

    bool recognizing_in_progress;
    int wait_time_ms;
    const int allowed_faces_count = 1;

    std::mutex mtx;
    std::shared_ptr<frame_keeper> keeper;
    AVFrame* get_frame_from_keeper();

    recognize_logger logger;
public:
    int prev_finded_faces;
    matcher face_matcher;
    explicit recognizer(const recognizer_settings& settings);
    ~recognizer();

    void convert_frame_to_mat(AVFrame *frame);
    int detect_faces();
    int alert_cheating(int faces_count);
    void recognize_faces();
    void display_mat();

    std::string run_recognizer_display();
    bool is_recognizing();
    void stop_cheking_display();

    void init();
    void setup_frame_keeper(const std::shared_ptr<frame_keeper>& keeper);
};


#endif  /* RECOGNIZER_HPP */
