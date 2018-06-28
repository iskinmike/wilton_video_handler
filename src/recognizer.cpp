#include "recognizer.hpp"

namespace {
#ifdef TEST_OUT
    std::fstream fs ("data.txt", std::fstream::out);
    const double shift_percent_threshold = 0.45;
#endif
    // this value determines intersection between new finded face and face search area of already created template
    const double intersection_percent_threshold = 0.85;
    // this value allows to detect face rotation from frontal to profile
    const double min_threshold = 0.20;

    double match_by_template_sqdif(const cv::Mat& frame, const cv::Mat& match_template,
                                   cv::Mat& matching_result, cv::Rect& out_area)
    {
        cv::matchTemplate(frame, match_template, matching_result, CV_TM_SQDIFF_NORMED);
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
        out_area = cv::Rect(minLoc.x, minLoc.y, match_template.cols, match_template.rows);
        return min;
    }

    double match_by_template_coeff(const cv::Mat& frame, const cv::Mat& match_template,
                                   cv::Mat& matching_result, cv::Rect& out_area)
    {
        cv::matchTemplate(frame, match_template, matching_result, CV_TM_CCOEFF_NORMED);
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
        out_area = cv::Rect(maxLoc.x, maxLoc.y, match_template.cols, match_template.rows);
        return max;
    }

    double match_by_template_corr(const cv::Mat& frame, const cv::Mat& match_template,
                                  cv::Mat& matching_result, cv::Rect& out_area)
    {
        cv::matchTemplate(frame, match_template, matching_result, CV_TM_CCORR_NORMED);
        double min, max;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(matching_result, &min, &max, &minLoc, &maxLoc);
        out_area = cv::Rect(maxLoc.x, maxLoc.y, match_template.cols, match_template.rows);
        return max;
    }
}

client_session::client_session(asio::io_service &io_service) : socket(io_service){}

client_session::~client_session(){
    close();
}
asio::ip::tcp::socket &client_session::get_socket(){
    return socket;
}
void client_session::send(std::string message, send_handler handler) {
    socket.async_send(asio::buffer(message), 0, handler);
}
void client_session::close(){
    socket.close();
}
bool client_session::is_open(){
    return socket.is_open();
}

bool alert_server::listen(){
    service_thread = std::thread([this] () {
        start_accept();
        io_service.run();
    });
}
void alert_server::start_accept() {
    std::shared_ptr<client_session> new_connection =
            std::make_shared<client_session> (acceptor.get_io_service());
    acceptor.async_accept(new_connection->get_socket(), [this, new_connection](const asio::error_code& error){
        this->sessions.insert(new_connection);
        start_accept();
    });
}
void alert_server::stop(){
    for (auto& el: sessions) {
        el->close();
    }
    io_service.stop();
    if (service_thread.joinable()){
        service_thread.join();
    }
}
void alert_server::send_cheating_alert_message(int faces_count){
    ++alert_number;
    std::string alert_message{};
    alert_message += "{\n  \"alert_number\" : ";
    alert_message += std::to_string(alert_number);
    alert_message += "\n  \"faces_count\" : ";
    alert_message += std::to_string(faces_count);
    alert_message += "\n}";

    for (auto& el: sessions) {
        auto handler = [this, el] (const asio::error_code& error, std::size_t bytes){
            if (error){
                el->close();
                this->sessions.erase(el);
            }
        };
        if (el->is_open()) {
            el->send(alert_message, handler);
        }
    }
}
void alert_server::send_error_message(std::string error){
    std::string error_message{};
    error_message += "{\n  \"error\" : ";
    error_message += error;
    error_message += "\n}";

    for (auto& el: sessions) {
        auto handler = [this, el] (const asio::error_code& error, std::size_t bytes){
            if (error){
                el->close();
                this->sessions.erase(el);
            }
        };
        if (el->is_open()) {
            el->send(error_message, handler);
        }
    }
}
void alert_server::drop_alerts(){
    alert_number = 0;
}
alert_server::alert_server(unsigned short port, std::string ip)
    : endpoint(asio::ip::address_v4::from_string(ip), port),
      acceptor(io_service, endpoint),
      alert_number(0)
{}
alert_server::~alert_server(){
    stop();
}

recognizer::recognizer(std::string ip, int port, std::string face_cascade_path, int64_t wait_time_ms) :
    frame(nullptr), stop_flag(false), alert_service(port, ip), recognizing_in_progress(false),
    face_cascade_path(face_cascade_path), wait_time_ms(wait_time_ms), prev_finded_faces(0)
{
    alert_service.listen();
}
recognizer::~recognizer(){
    alert_service.stop();
}
void recognizer::convert_frame_to_mat(){
    AVFrame dst;
    struct SwsContext* sws_ctx;

    int w = frame->width;
    int h = frame->height;
    frame_mat = cv::Mat(h, w, CV_8UC3);
    dst.data[0] = (uint8_t *)frame_mat.data;
    avpicture_fill( (AVPicture *)&dst, dst.data[0], AV_PIX_FMT_BGR24, w, h);

    enum PixelFormat src_pixfmt = (enum PixelFormat)frame->format;
    enum PixelFormat dst_pixfmt = AV_PIX_FMT_BGR24;
    sws_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);

    sws_scale(sws_ctx, frame->data, frame->linesize, 0, h,
              dst.data, dst.linesize);
}
int recognizer::detect_faces(){
    std::vector<cv::Rect> faces;
    cv::Mat frame_gray;

    cv::cvtColor( frame_mat, frame_gray, CV_BGR2GRAY );
    cv::equalizeHist( frame_gray, frame_gray );

    // face detect by Haar
    face_cascade.detectMultiScale( frame_gray, faces, 1.2, 3, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));

#ifdef TEST_OUT
    std::vector<cv::Rect> haar_faces;
    haar_faces = faces;
    std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    fs << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    fs << "finded faces templates: " << face_matcher.get_found_faces_count() << std::endl;
    fs << "finded faces haar: " << faces.size() << " | " << prev_finded_faces << std::endl;
#endif

    // update templates if find faces by haar
    if (faces.size()) {
        face_matcher.update_templates(faces, frame_mat);
    }

    face_matcher.remove_intersected_templates();

    if (face_matcher.get_found_faces_count() != faces.size()) {
        face_matcher.search_faces(faces, frame_mat);
    }

#ifdef TEST_OUT
    // Remember all finded faces
    prev_finded_faces = faces.size();
    fs << "------------------------------------------------------------------" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    for (auto& el: faces) {
        cv::rectangle(frame_mat,
                      cv::Rect(
                          el.x,
                          el.y,
                          el.width,
                          el.height),
                      cv::Scalar( 0, 0, 255 ),
                      2,5,0);
    }

    for( size_t i = 0; i < haar_faces.size(); i++ )
    {
        cv::Point center( haar_faces[i].x + haar_faces[i].width*0.5, haar_faces[i].y + haar_faces[i].height*0.5 );
        cv::ellipse( frame_mat, center, cv::Size( haar_faces[i].width*0.5, haar_faces[i].height*0.5), 0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0 );
    }
#endif

    return faces.size();
}
int recognizer::alert_cheating(int faces_count){
    alert_service.send_cheating_alert_message(faces_count);
}
void recognizer::recognize_faces(){
    int faces_count = detect_faces();
    if (faces_count != allowed_faces_count) {
        alert_cheating(faces_count);
    }
}
void recognizer::display_mat(){
    recognizing_in_progress = true;

    frame_keeper& fk = frame_keeper::instance();
#ifdef TEST_OUT
    cv::startWindowThread();
    std::string window_name{"MyVideo"};
    cv::namedWindow( window_name, cv::WINDOW_AUTOSIZE);
    cv::moveWindow(window_name, 1000, 200);
#endif
    while (!stop_flag) {
        frame = fk.get_frame();
        if (nullptr != frame) {
            convert_frame_to_mat();
            recognize_faces();
#ifdef TEST_OUT
            cv::imshow(window_name, frame_mat);
#endif
        } else {
            alert_service.send_error_message("Get NULL 'frame' from video device. Run decoder to obtain frames.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }
#ifdef TEST_OUT
    cv::destroyWindow(window_name);
#endif
    stop_flag.exchange(false);
}

std::string recognizer::run_recognizer_display(){
    if( !face_cascade.load( face_cascade_path ) ){
        return std::string{"Can't load face cascade data. Face recognition not working."};
    };
    recognizer_display_thread = std::thread([this] (){
        this->display_mat();
    });
    return std::string{};
}

bool recognizer::is_recognizing(){
    return recognizing_in_progress;
}

void recognizer::stop_cheking_display() {
    stop_flag.exchange(true);
    if (recognizer_display_thread.joinable()) {
        recognizer_display_thread.join();
    }
    recognizing_in_progress = false;
}


// matcher
#ifdef TEST_OUT
uint64_t template_detector::next_id(0);
#endif


void matcher::update_templates(std::vector<cv::Rect> &areas, const cv::Mat &image) {
#ifdef TEST_OUT
    fs /*std::cout*/ << "area size after: " << areas.size() << std::endl;
#endif
    for (auto& area: areas) {
        auto new_template = std::make_shared<template_detector>(template_detector(image, area));
        new_templates.insert(new_template);
    }
}

void matcher::search_faces(std::vector<cv::Rect> &faces, const cv::Mat& image){
#ifdef TEST_OUT
    fs /*std::cout*/ << " ======= search_faces" << std::endl;
#endif
    std::vector<cv::Rect> finded_faces;
    // check all templates
    for (auto detector = templates.begin(); detector != templates.end();) {
        cv::Rect face = (*detector)->match(image);
        // If can't find face - delete template;
        if ((*detector)->is_face_detected) {
            finded_faces.push_back(face);
            (*detector)->update_template(image, face); // need to update in opposite can't detect face shift.
        } else {
            detector = templates.erase(detector);
            continue;
        }
        ++detector;
    }

    faces.clear();
    for (auto& el : finded_faces) {
        faces.push_back(el);
    }
}

void matcher::remove_intersected_templates(){
#ifdef TEST_OUT
    fs << " ======= remove_intersected_templates: " << templates.size() << std::endl;
#endif
    if (new_templates.empty()) {
        return;
    }

    std::set<std::shared_ptr<template_detector>> templates_to_delete;

    for (auto& new_tmplt: new_templates) {
        cv::Rect new_tmplt_area = new_tmplt->tracked_face_area;
        for (auto old_tmplt_it = templates.begin(); old_tmplt_it != templates.end(); ++old_tmplt_it){
            cv::Rect old_tmplt_area = old_tmplt_it->get()->tracked_face_area;
            cv::Rect intersec = new_tmplt_area & old_tmplt_area;

            double first_s = new_tmplt_area.width*new_tmplt_area.height;
            double second_s = old_tmplt_area.width*old_tmplt_area.height;
            double intersec_s = intersec.width*intersec.height;

            double first_intersect_percent =  intersec_s / first_s;
            double second_intersect_percent =  intersec_s / second_s;

            bool first_above_threshold = (intersection_percent_threshold < first_intersect_percent);
            bool second_above_threshold = (intersection_percent_threshold < second_intersect_percent);

#ifdef TEST_OUT
            fs  << "first second S: " <<  first_s << " | " << second_s << std::endl;
            fs  << "**** other template: [" << old_tmplt_it->get()->uniq_id << "] | | " <<
                   first_above_threshold << " | " << second_above_threshold << " --- " <<
                   first_intersect_percent << " | " << second_intersect_percent <<std::endl;
#endif
            if (first_above_threshold || second_above_threshold) {
                templates_to_delete.insert(*old_tmplt_it);
            }
        }
    }


    for (auto detector = templates.begin(); detector != templates.end();) {
        if (templates_to_delete.count(*detector)) {
#ifdef TEST_OUT
            fs /*std::cout*/ << "------ remove intersected template: ["  << detector->get()->uniq_id << "]"<< std::endl;
#endif
            detector = templates.erase(detector);
        } else {
            ++detector;
        }
    }

    for (auto& new_tmplt: new_templates) {
        templates.insert(new_tmplt);
    }
    new_templates.clear();
}

int matcher::get_found_faces_count(){
    return templates.size();
}

template_detector::template_detector(const cv::Mat &image, const cv::Rect &face): is_face_detected(false) {
    update_template(image, face);
    origin_template = compare_template;
#ifdef TEST_OUT
    uniq_id = next_id++;
#endif
}

void template_detector::set_prev_point(const cv::Point &point){
    prev_point = point;
}

void template_detector::drop_prev_point(){
    prev_point = cv::Point(0,0);
}

void template_detector::update_template(const cv::Mat &frame, cv::Rect face) {
    tracked_face_area = double_size(face, cv::Rect(0,0,frame.cols, frame.rows));
    compare_template = frame(face).clone();

    face.x += face.width / 4;
    face.y += face.height / 4;
    face.width /= 2;
    face.height /= 2;

    face_template = frame(face).clone();

    set_prev_point(cv::Point(face.x, face.y));
}

cv::Rect template_detector::double_size(cv::Rect view, cv::Rect border_rect) {
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

cv::Rect template_detector::match(cv::Mat frame){
    double min = match_by_template_sqdif(frame(tracked_face_area), face_template, matching_result, face_area);
    face_area.x += tracked_face_area.x;
    face_area.y += tracked_face_area.y;
    face_area = double_size(face_area, cv::Rect(0,0,frame.cols, frame.rows));

    cv::Point minLoc(face_area.x, face_area.y);
    double distance = calc_vector(minLoc);

    if (min_threshold > min) {
        is_face_detected = true;
        percent = min;
    } else {
        is_face_detected = false;
    }

#ifdef TEST_OUT
    double shift_percent = distance/tracked_face_area.width;
    fs /*std::cout*/ << "template id [" << this->uniq_id << "]: " << std::endl;
    fs /*std::cout*/ << "Mins shifted: " << minLoc.x << " | " << minLoc.y << " | " << min << " | " << distance << std::endl;
    fs /*std::cout*/ << "---- test Mins shifted: " << distance << " | " << tracked_face_area.width << " | " << shift_percent << std::endl;
    if (shift_percent > shift_percent_threshold) {
        fs /*std::cout*/ << "---- test real shift: " << std::endl;
        fs /*std::cout*/ << "---- old point: " << prev_point.x << " | " << prev_point.y << " | " /*<< shift_percent*/ << std::endl;
        fs /*std::cout*/ << "---- new point: " << face_area.x << " | " << face_area.y << " | " /*<< shift_percent*/ << std::endl;
    }
#endif
    return face_area;
}

double template_detector::calc_vector(const cv::Point &point){
    return cv::norm(point - prev_point);
}
