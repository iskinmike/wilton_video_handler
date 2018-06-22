#include "recognizer.hpp"

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
//    fs.open();
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
    std::vector<cv::Rect> haar_faces;
    cv::Mat frame_gray;

    cv::cvtColor( frame_mat, frame_gray, CV_BGR2GRAY );
    cv::equalizeHist( frame_gray, frame_gray );

    // детектим через Haar
    face_cascade.detectMultiScale( frame_gray, faces, 1.2, 3, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));


//    prev_finded_faces;
    haar_faces = faces;
    // Если нашли хотя бы одно лицо, то делаем update по найденным
    std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    fs /*std::cout*/ << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    fs /*std::cout*/ << "finded faces templates: " << face_matcher.get_found_faces_count() << std::endl;
    fs /*std::cout*/ << "finded faces haar: " << faces.size() << " | " << prev_finded_faces << std::endl;
    if (faces.size()) {
        face_matcher.update_templates(faces, frame_mat);
//        detector.update_template(frame_mat, faces[0]);
    }
//    if (prev_finded_faces == faces.size()) {
//        face_matcher.remove_unmatched_templates(faces);
//    }
    if (/*(face_matcher.get_found_faces_count() != prev_finded_faces) && */face_matcher.get_found_faces_count()) {
        face_matcher.remove_intersected_templates();
    }

    // Теперь, если количество лиц не совпадает с количеством ранее найденных
    // Вообще надо бы не по предыдущим найденным сравнивать а по количеству шаблонов.
    if (face_matcher.get_found_faces_count() != faces.size()) {
        face_matcher.search_faces(faces, frame_mat);
    }

    // Запомним все найденные лица
    prev_finded_faces = faces.size();
    fs /*std::cout*/ << "------------------------------------------------------------------" << std::endl;
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
    cv::startWindowThread();
    std::string window_name{"MyVideo"};
    cv::namedWindow( window_name, cv::WINDOW_AUTOSIZE);
    cv::moveWindow(window_name, 1000, 200);
    while (!stop_flag) {
        frame = fk.get_frame();
        if (nullptr != frame) {
            convert_frame_to_mat();
            recognize_faces();
            cv::imshow(window_name, frame_mat);
        } else {
            alert_service.send_error_message("Get NULL 'frame' from video device. Run decoder to obtain frames.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }
    cv::destroyWindow(window_name);
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

void recognizer::stop_cheking_display() {
    stop_flag.exchange(true);
    if (recognizer_display_thread.joinable()) {
        recognizer_display_thread.join();
    }
    recognizing_in_progress = false;
}
