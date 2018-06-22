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

using send_handler = std::function<void(const asio::error_code&, std::size_t)>;


namespace {
    std::fstream fs ("data.txt", std::fstream::out);
    const double intersection_percent_threshold = 0.60;
    const double templates_match_threshold = 0.15;

    const double min_threshold = 0.15;
    const double min_distance_shift = 1000;

    const double min_same_template_threshold = 0.15;

    std::string data;
    std::ostringstream ostream;
    uint64_t id = 0;
}

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
    // template matching
    cv::Mat face_template;
    cv::Mat compare_template;
    cv::Mat matching_result;

    cv::Rect face_area;
    cv::Rect tracked_face_area;

    bool is_prev_point_setted;
    cv::Point prev_point;

    bool is_face_detected;
    double percent;
    int counter;

    bool is_template_setted;

    uint64_t uniq_id;

//    cv::matchTemplate(frame(m_faceRoi), m_faceTemplate, m_matchingResult, CV_TM_SQDIFF_NORMED);
    void setup_template_from_frame_area(cv::Mat frame) {
        face_template;
    }

    template_detector(): is_prev_point_setted(false), is_face_detected(false), percent(0.0f), counter(0), is_template_setted(false) {
        uniq_id = id++;
    }
    template_detector(const cv::Mat& image, const cv::Rect& face): is_prev_point_setted(false), is_face_detected(false), counter(0), is_template_setted(false) {
        update_template(image, face);
        uniq_id = id++;
    }

    void set_prev_point(const cv::Point& point){
        prev_point = point;
        is_prev_point_setted = true;
    }

    void drop_prev_point(){
        prev_point = cv::Point(0,0);
        is_prev_point_setted = false;
    }

    void update_template(const cv::Mat &frame, cv::Rect face) {
//        tracked_face_area.width = face.width*2;
//        tracked_face_area.height = face.height*2;

        // Center rect around original center
//        tracked_face_area.x = face.x - face.width / 2;
//        tracked_face_area.y = face.y - face.height / 2;

        tracked_face_area = double_size(face, cv::Rect(0,0,frame.cols, frame.rows));
//        tracked_face_area = double_size(tracked_face_area, cv::Rect(0,0,frame.cols, frame.rows));

        compare_template = frame(face).clone();

        face.x += face.width / 4;
        face.y += face.height / 4;
        face.width /= 2;
        face.height /= 2;

        face_template = frame(face).clone();

        is_template_setted = true;

        set_prev_point(cv::Point(face.x + face.width/2, face.y + face.height/2));
    }

    cv::Rect double_size(cv::Rect view, cv::Rect border_rect) {
        cv::Rect resized;

        resized.width = view.width*2;
        resized.height = view.height*2;

        // Center rect around original center
        resized.x = view.x - view.width / 2;
        resized.y = view.y - view.height / 2;

        if (border_rect.width < resized.width) resized.width = border_rect.width;
        if (border_rect.height < resized.height) resized.height = border_rect.height;

        if (0 > resized.x) resized.x = 0;
        if (0 > resized.y) resized.y = 0;

        if (border_rect.width < (resized.x + resized.width)) resized.x = border_rect.width - resized.width;
        if (border_rect.height < (resized.y + resized.height)) resized.y = border_rect.height - resized.height;

        return resized;
    }

    cv::Rect match(cv::Mat frame){
        cv::matchTemplate(frame(tracked_face_area), face_template, matching_result, CV_TM_SQDIFF_NORMED);
//        cv::normalize(matching_result, matching_result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
        // Add roi offset to face position

//        cv::normalize(matching_result, matching_result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
//        cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
        fs /*std::cout*/ << "template id [" << this->uniq_id << "]: " << std::endl;
        fs /*std::cout*/ << "Mins: " << minLoc.x << " | " << minLoc.y << " | " << min << std::endl;
        minLoc.x += tracked_face_area.x;
        minLoc.y += tracked_face_area.y;

        cv::Rect old_face_area = face_area;
        face_area = cv::Rect(minLoc.x, minLoc.y, face_template.cols, face_template.rows);
        face_area = double_size(face_area, cv::Rect(0,0,frame.cols, frame.rows));


        double distance = calc_vector(minLoc);
        fs /*std::cout*/ << "Mins shifted: " << minLoc.x << " | " << minLoc.y << " | " << min << " | " << distance << std::endl;

        if (min_threshold > min/* && (min_distance_shift > distance)*/) {
            is_face_detected = true;
            percent = min;
        } else {
            is_face_detected = false;
        }

        if (distance > 80) {
            // Нужно посмотреть какие тут будут запросы.
            // Ну например  уменьшить область и посмотреть будет ли в ней тоже подходящий шаблон.
            cv::Rect tmp_area = tracked_face_area;
            tmp_area.width = face_template.cols;
            tmp_area.height = face_template.rows;

            tmp_area.x += (tracked_face_area.width - tmp_area.width)/2;
            tmp_area.y += (tracked_face_area.height - tmp_area.height)/2;

            cv::matchTemplate(frame(tmp_area), face_template, matching_result, CV_TM_SQDIFF_NORMED);
            cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
            minLoc.x += tmp_area.x;
            minLoc.y += tmp_area.y;
            distance = calc_vector(minLoc);
            fs /*std::cout*/ << "---- test Mins shifted: " << minLoc.x << " | " << minLoc.y << " | " << min << " | " << distance << std::endl;

            cv::Rect intersection = old_face_area | face_area;
            double intersection_s = intersection.width*intersection.height;
            double old_face_s = old_face_area.width*old_face_area.height;
            // Можно найти пересчение для face_area новой и старой
            fs /*std::cout*/ << "---- test intersection: " << intersection_s << " | " << old_face_s << " | " << old_face_s/intersection_s << " | " << std::endl;
        }

        return face_area;
    }

    double calc_vector(const cv::Point& point){
        return cv::norm(point - prev_point);
    }

    bool has_same_area(const cv::Rect& area) {
        int x_left, y_left, x_right, y_right;
        int x,y, x_r, y_r;

        x_left = area.x;
        y_left = area.y;
        x_right = area.x + area.width;
        y_right = area.y + area.height;

        // cv::Point2d left_top_point(x_left, y_left);
        // cv::Point2d left_bottom_point(x_left, y_right);
        // cv::Point2d right_top_point(x_right, y_left);
        // cv::Point2d right_bottom_point(x_right, y_right);

        // bool left_top_point_in = tracked_face_area.contains(left_top_point);
        // bool left_bottom_point_in = tracked_face_area.contains(left_bottom_point);
        // bool right_top_point_in = tracked_face_area.contains(right_top_point);
        // bool right_bottom_point_in = tracked_face_area.contains(right_bottom_point);

        // fs << "left_top_point     contains:  "  << left_top_point_in     << std::endl;
        // fs << "left_bottom_point  contains:  "  << left_bottom_point_in  << std::endl;
        // fs << "right_top_point    contains:  "  << right_top_point_in    << std::endl;
        // fs << "right_bottom_point contains:  "  << right_bottom_point_in << std::endl;
        // fs /*std::cout*/ << "left point contains:  "  << left_point_in << std::endl;
//                     tracked_face_area.contains(cv::Point2d(x_left,y_left)) << std::endl;
        // fs /*std::cout*/ << "right point contains:  "  << right_point_in << std::endl;
//                     tracked_face_area.contains(cv::Point2d(x_right,y_right)) << std::endl;

        x = tracked_face_area.x;
        y = tracked_face_area.y;
        x_r = tracked_face_area.x + tracked_face_area.width;
        y_r = tracked_face_area.y + tracked_face_area.height;

        fs /*std::cout*/ << "area: "  << x_left << "|" << y_left << "|" << x_right << "|" << y_right << "|" << std::endl;
        fs /*std::cout*/ << "tracked_face_area[" << this->uniq_id << "]: "  << x << "|" << y <<  "|" << x_r << "|" << y_r << "|" << std::endl;

        // if (left_top_point_in && left_bottom_point_in && right_top_point_in && right_bottom_point_in){
            // return true;
        // } else if (left_top_point_in || left_bottom_point_in || right_top_point_in || right_bottom_point_in) {
            // считаем пересечение
        cv::Rect cross = area & tracked_face_area;
        int cr_x_l, cr_x_r;
        int cr_y_l, cr_y_r;

        cr_x_l = cross.x;
        cr_y_l = cross.y;
        cr_x_r = cross.x + cross.width;
        cr_y_r = cross.y + cross.height;

        fs /*std::cout*/ << "cross: " << cr_x_l << "|" << cr_y_l << "|" << cr_x_r << "|" << cr_y_r << "|" << std::endl;
//            const double intersection_percent_threshold = 0.70;
        double cross_s = cross.width*cross.height;
        double area_s = area.width*area.height;
        double intersection_percent =  cross_s / area_s;
        fs /*std::cout*/ << "cross S: " << cross_s << std::endl;
        fs /*std::cout*/ << "area S: " << area_s << std::endl;
        fs /*std::cout*/ << "intersection_percent: " << intersection_percent << std::endl;
        if (intersection_percent_threshold < intersection_percent){
            return true;
        }

        return false;
    }


     bool has_same_area_by_template(const cv::Mat& frame, const cv::Rect& area) {

         cv::Mat tmp_frame = frame(area).clone();

         cv::Mat result;

         cv::matchTemplate(tmp_frame, compare_template, result, CV_TM_SQDIFF_NORMED);
         double min, max;
         cv::Point minLoc, maxLoc;
         cv::minMaxLoc(result, &min, &max, &minLoc, &maxLoc);
         fs /*std::cout*/ << "similar probability : " << min << std::endl;
         if (min < min_same_template_threshold) {
             return true;
         }

         return this->has_same_area(area);
//         return false;
     }
};

class matcher
{
    std::set<std::shared_ptr<template_detector>> templates;
    std::set<uint64_t> new_templates;
public:
    matcher(/*const std::vector<template_detector>& templates*/) /*: templates(templates)*/ {
    }
    ~matcher(){
    }

    void update_templates(std::vector<cv::Rect> areas, cv::Mat image) {
//        std::set<cv::Rect*> new_areas;
//        fs /*std::cout*/ << " ======= update_templates" << std::endl;

//        for (auto& area: areas) {
//            fs /*std::cout*/ << "update area: " << area.x << "|" << area.y << "|" << area.width << "|" << area.height << "|" << std::endl;
//        }
//        for (auto it = areas.begin(); it != areas.end();) {
//            fs /*std::cout*/ << "------ update current area: " << it->x << "|" << it->y << "|" << it->width << "|" << it->height << "|" << std::endl;
//            bool erased_flag = false;
//            for (auto detector: templates) {
//                fs /*std::cout*/ << "template [" << detector->uniq_id << "]" << std::endl;
//                if (detector->has_same_area_by_template(image, *it)) {
////                if (detector->has_same_area(*it)) {
//                    fs << "------ updated: " << std::endl;
//                    detector->update_template(image, *it);
//                    erased_flag = true;
//                    break; // exit because iterators broken;
//                }
//            }
//            if (erased_flag) {
//                it = areas.erase(it);
//            } else {
//                ++it;
//            }
//        }

        fs /*std::cout*/ << "area size after: " << areas.size() << std::endl;
        for (auto& area: areas) {
            auto new_template = std::make_shared<template_detector>(template_detector(image, area));
            templates.insert(new_template);
            new_templates.insert(new_template->uniq_id);
        }
    }

    void search_faces(std::vector<cv::Rect>& faces, cv::Mat image){
        fs /*std::cout*/ << " ======= search_faces" << std::endl;
        std::vector<cv::Rect> finded_faces;
//        std::vector<std::shared_ptr<template_detector>> unused_templates;
//        for (auto area = faces.begin(); area != faces.end(); ++area) {
//            for (auto detector = templates.begin(); detector != templates.end(); ++detector) {
//                if ((*detector)->has_same_area(*area)) {
//                    unused_templates.push_back(*detector);
//                    templates.erase(detector);
//                    break;
//                }
//            }
//        }
        // пробегаемся по оставшимся шаблонам
        for (auto detector = templates.begin(); detector != templates.end();) {
            // Тогда делаем функцию поиска у template по image
            cv::Rect face = (*detector)->match(image);
            // Тпереь в зависимости от того что нашли либо записываем лицо и итерируем дальше, либо удаляем детектор, при этом итерируя.
            if ((*detector)->is_face_detected) {
                finded_faces.push_back(face);
//                if ((*detector)->percent < 0.05) {
                    (*detector)->update_template(image, face);
//                }
            } else {
                detector = templates.erase(detector);
                continue;
            }
            ++detector;
        }

        // Теперь в оставшиеся необходимо закинуть неиспользуемые
//        for (auto& el : unused_templates) {
//            templates.insert(el);
//        }
        faces.clear();
        for (auto& el : finded_faces) {
            faces.push_back(el);
        }
    }

    void remove_unmatched_templates(const std::vector<cv::Rect>& faces){
//        std::vector<std::shared_ptr<template_detector>> unused_templates;
        fs /*std::cout*/ << " ======= remove_unmatched_templates" << std::endl;
        for (auto detector = templates.begin(); detector != templates.end(); /*++detector*/) {
            fs /*std::cout*/ << "------ test template: " << 
                    detector->get()->tracked_face_area.x << "|" << detector->get()->tracked_face_area.y << "|" <<
                    detector->get()->tracked_face_area.width << "|" << detector->get()->tracked_face_area.height << "|" << std::endl;
            bool erase_flag = true;
            for (auto area = faces.begin(); area != faces.end(); ++area) {
                if ((*detector)->has_same_area(*area)) {
//                    unused_templates.push_back(*detector);
                    erase_flag = false;
                    break;
                }
            }
            if (erase_flag) {
                detector = templates.erase(detector);
            } else {
                ++detector;
            }
        }
    }

    void remove_intersected_templates(){
        fs /*std::cout*/ << " ======= remove_intersected_templates: " << templates.size() << std::endl;
        std::set<std::shared_ptr<template_detector>> templates_to_delete;
//        bool erase_first_flag = true;
//        bool erase_second_flag = true;
        for (auto detector = templates.begin(); detector != templates.end();) {
            auto end_test = detector;
            ++end_test;
            if (end_test == templates.end()) break;
            auto first_it = detector;
            ++detector;
            fs  << "------ test template: [" << first_it->get()->uniq_id << "]" << std::endl;
            cv::Rect first = first_it->get()->tracked_face_area;
            for (auto second_it = detector; second_it != templates.end(); ++second_it){
                cv::Rect second = second_it->get()->tracked_face_area;
                cv::Rect intersec = first & second;

                double first_s = first.width*first.height;
                double second_s = second.width*second.height;
                double intersec_s = intersec.width*intersec.height;

                double first_intersect_percent =  intersec_s / first_s;
                double second_intersect_percent =  intersec_s / second_s;

                bool first_above_threshold = (intersection_percent_threshold < first_intersect_percent);
                bool second_above_threshold = (intersection_percent_threshold < second_intersect_percent);

                fs  << "first second S: " <<  first_s << " | " << second_s << std::endl;
                fs  << "**** other template: [" << second_it->get()->uniq_id << "] | | " <<
                       first_above_threshold << " | " << second_above_threshold << " --- " <<
                       first_intersect_percent << " | " << second_intersect_percent <<std::endl;

//                if (first_above_threshold && second_above_threshold) {
//                    // Удаляем тот у которого меньше площадь.
//                    if (first_s < second_s) {
//                        templates_to_delete.insert(*first_it);
//                    } else {
//                        templates_to_delete.insert(*second_it);
//                    }
//                }

                // Сравним теперь шаблоны если никого удалять не стали
                cv::Mat first_template = first_it->get()->compare_template;
                cv::Mat second_template = second_it->get()->compare_template;
                cv::Mat matching_result;

                cv::matchTemplate(first_template, second_template, matching_result, CV_TM_SQDIFF_NORMED);
                double min, max;
                cv::Point minLoc, maxLoc;
                cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);

                fs << "templates probability: " << min << " | " << std::endl;

                if ((templates_match_threshold > min) || (first_above_threshold && second_above_threshold)) {
                    // Нужно удалить тот который раньше появился
//                    if (first_s < second_s) {
//                    if (new_templates.count(first_it->get()->uniq_id)) {
                    if (first_it->get()->uniq_id > second_it->get()->uniq_id) {
                        templates_to_delete.insert(*second_it);
                    } else {
                        templates_to_delete.insert(*first_it);
                    }
                } else {
                    if (first_above_threshold && !second_above_threshold) {
                        // Удаляем первый
                        templates_to_delete.insert(*first_it);
                    } else if (!first_above_threshold && second_above_threshold) {
                        // Удаляем второй
                        templates_to_delete.insert(*second_it);
                    }
                }

            }
        }

        for (auto detector = templates.begin(); detector != templates.end();) {
            if (templates_to_delete.count(*detector)) {
                fs /*std::cout*/ << "------ remove intersected template: ["  << detector->get()->uniq_id << "]"<< std::endl;
                detector = templates.erase(detector);
            } else {
                ++detector;
            }
        }
    }

    int get_found_faces_count(){
        return templates.size();
        bool erase_flag = true;
    }

};

class recognizer
{
    AVFrame* frame;
    cv::Mat frame_mat;

    std::atomic_bool stop_flag;
    std::thread recognizer_display_thread;
    cv::CascadeClassifier face_cascade;
//    cv::CascadeClassifier eyes_cascade;
    cv::String face_cascade_path;
//    cv::String eyes_cascade_name;

    alert_server alert_service;
    template_detector detector;

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
    bool is_recognizing(){
        return recognizing_in_progress;
    }
    void stop_cheking_display();

    void init();
};


#endif  /* RECOGNIZER_HPP */
