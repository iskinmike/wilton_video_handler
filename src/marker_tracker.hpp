#ifndef MARKER_TRACKER_HPP
#define MARKER_TRACKER_HPP


extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h> // installed libavdevice-dev
}

#include <ARX/ARController.h>

struct marker_tracker_data{
    int frame_width;
    int frame_height;
};

struct preconstruct_marker_tracker_token{
    ARController *controller;
    marker_tracker_data data;
    int error_state;
};

class marker_tracker
{
    ARController *controller;
    std::shared_ptr<ARTrackerSquare> sq_tracker;
    ARTrackable* trackable;
    int track_uid;
    std::vector<ARTrackable *> trackables;

    marker_tracker_data data;
    marker_tracker(ARController *controller): controller(controller) {}
public:
    marker_tracker(preconstruct_marker_tracker_token&& token): controller(token.controller), data(token.data) {
//        track_uid = controller->addTrackable("single;/home/mike/workspace/tmp/artoolkitx_test/resources/hiro.patt;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/Examples/Square tracking example/Linux/build/../share/artoolkitx_square_tracking_example/hiro.patt;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/3.patt;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/3-00.pat;80.0");
        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/zero-00.pat;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/letter_b-00.pat;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/letter_b_colored-00.pat;80.0");
//        track_uid = controller->addTrackable("single;/home/mike/workspace/utils/artoolkitx/SDK/bin/b_blue_full-00.pat;80.0");
        trackable = controller->findTrackable(track_uid);
        sq_tracker = controller->getSquareTracker();
//        controller->projectionMatrix();
        trackables.push_back(trackable);
        ARParam cparam;
        arParamClearWithFOVy(&cparam, data.frame_width, data.frame_height, M_PI_4); // M_PI_4 radians = 45 degrees.
        ARParamLT* param = nullptr;
        param = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET);
        sq_tracker->setPatternDetectionMode(AR_TEMPLATE_MATCHING_COLOR);
//        sq_tracker->setLabelingMode(AR_LABELING_WHITE_REGION);
//        sq_tracker->setLabelingMode(AR_LABELING_WHITE_REGION);
//        sq_tracker->setPatternSize(16);

//        arSetPatternDetectionMode(arHandle, AR_MATRIX_CODE_DETECTION);
//        arSetMatrixCodeType(arHandle, AR_MATRIX_CODE_3x3);
//        sq_tracker->setMatrixCodeType(AR_MATRIX_CODE_3x3);
        sq_tracker->setThresholdMode(AR_LABELING_THRESH_MODE_AUTO_BRACKETING);
//        sq_tracker->setThresholdMode(AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE);

//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_yuvs); // AV_PIX_FMT_YUV420P in ffmpeg equal AR_PIXEL_FORMAT_yuvs in ARToolKit
//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_BGRA); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_2vuy); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_ARGB); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_420v); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_BGRA); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
//        bool res = sq_tracker->start(param, AR_PIXEL_FORMAT_BGRA); // AV_PIX_FMT_YUYV422 in ffmpeg equal AR_PIXEL_FORMAT_2vuy in ARToolKit
        std::cout << "*** tracker started: " << res << std::endl;
        std::cout << "*** cparam.xsize: " << cparam.xsize << std::endl;
        std::cout << "*** cparam.ysize: " << cparam.ysize << std::endl;

        ARHandle *m_arHandle0 = arCreateHandle(param);
        m_arHandle0;

    }
    ~marker_tracker(){
        controller->shutdown();
    }

    static preconstruct_marker_tracker_token preconstruction() noexcept{
        preconstruct_marker_tracker_token token;
        ARController *c = new ARController();
        token.error_state = 0;
        token.controller = c;
        token.data.frame_width = 640;// 1280;
        token.data.frame_height = 480;// 720;
        if (!c->initialiseBase()) {
            ARLOGe("Error initialising ARController.\n");
            token.error_state = 1;
        }
        return token;
    }

    static AR2VideoBufferT frame_to_buffer(AVFrame* frame){
        AR2VideoBufferT buffer;
        buffer.bufPlaneCount = 0;
        buffer.fillFlag = 1;
        buffer.buff = static_cast<ARUint8*>(frame->data[0]);
        buffer.buffLuma = buffer.buff;
        buffer.bufPlanes = nullptr;//&buffer.buff;
        return buffer;
    }

    std::vector<float> find_tracked_transform_matrix(AVFrame* frame, bool& state_upd, bool& state_vis){
        AR2VideoBufferT ar_buffer = frame_to_buffer(frame);
        state_upd = sq_tracker->update(&ar_buffer, trackables);
        ARLOGd("state_upd.\n");
        std::vector<float> view(16);
        if (trackable->visible) {
            //arUtilPrintMtx16(marker->transformationMatrix);
            for (int i = 0; i < 16; i++) view[i] = (float)trackable->transformationMatrix[i];
            state_vis = trackable->visible;
//            arUtilPrintMtx16(trackable->transformationMatrix);
        }
        return view;
    }
};


AVFrame* draw_rect(AVFrame* frame, int x, int y);

//ARController *create_tracker();


#endif  /* MARKER_TRACKER_HPP */
