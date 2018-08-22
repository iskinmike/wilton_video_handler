/* 
 * Copyright 2018, alex at staticlibs.net
 * Copyright 2018, mike at myasnikov.mike@gmail.com
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

define([
    "wilton/dyload",
    "wilton/wiltoncall",
    "wilton/thread",
    "wilton/Logger"
], function(dyload, wiltoncall, thread, Logger) {
    "use strict";

    // load shared lib on init
    dyload({
        name: "wilton_video_handler"
    });
    var logger = new Logger("server.main");
    
    return {
        main: function() {
            Logger.initConsole("INFO");
            print("Calling native module ...");
            var decoder_settings = {};
            decoder_settings["id"] = 1;
            decoder_settings["in"] = "/dev/video0";   // linux
            decoder_settings["fmt"] = "video4linux2"; // linux
            // decoder_settings["in"] = "video=HP Webcam";  // windows
            // decoder_settings["fmt"] = "dshow";           // windows
            decoder_settings["time_base_den"] = 1000000;
            decoder_settings["time_base_num"] = 1;

            var decoder_settings_2 = {};
            decoder_settings_2["id"] = 2;
            decoder_settings_2["in"] = "/dev/video1";   // linux
            decoder_settings_2["fmt"] = "video4linux2"; // linux
            decoder_settings_2["time_base_den"] = 10000000;
            decoder_settings_2["time_base_num"] = 10;

            var encoder_settings = {};
            encoder_settings["id"] = 1;
            encoder_settings["out"] = "out.mp4";
            encoder_settings["width"] = 480;
            encoder_settings["height"] = 360;
            encoder_settings["bit_rate"] = 100000;
            encoder_settings["framerate"] = parseFloat("10.0");
            
            var display_settings = {};
            display_settings["id"] = 1;
            display_settings["title"] = "CAM";
            display_settings["width"] = 320;
            display_settings["height"] = 240;
            display_settings["pos_x"] = 300;
            display_settings["pos_y"] = 300;

            var photo_settings = {};
            photo_settings["id"] = 1;
            photo_settings["width"] = 640;
            photo_settings["height"] = 480;
            photo_settings["photo_name"] = "photo.png";

            // for (var counter = 0; counter < 10; ++counter) {
            var decoder_id = wiltoncall("av_init_decoder", decoder_settings);
            print("av_init_decoder");
            var encoder_id = wiltoncall("av_init_encoder", encoder_settings);
            print("av_init_encoder");
            var display_id = wiltoncall("av_init_display", display_settings);
            print("av_init_display");

            wiltoncall("av_setup_decoder_to_display", {decoder_id: 1, display_id: 1});
            wiltoncall("av_setup_decoder_to_encoder", {decoder_id: 1, encoder_id: 1});
            print("av_setup_decoder_to_encoder");
            var start_res = wiltoncall("av_start_decoding", decoder_id);
            print("start_res: " + start_res); 
            start_res = wiltoncall("av_start_encoding", encoder_id);
            print("start_res: " + start_res); 
            start_res = wiltoncall("av_start_video_display", display_id);
            print("start_res: " + start_res); 
            for (var i = 0; i < 2; ++i) {
                logger.info("Server is running ...");
                thread.sleepMillis(1000);
            }
            
            var decoder_id_2 = wiltoncall("av_init_decoder", decoder_settings_2);
            print("av_init_decoder_2");
            start_res = wiltoncall("av_start_decoding", decoder_id_2);
            print("start_res 2: " + start_res);
            wiltoncall("av_setup_decoder_to_display", {decoder_id: 2, display_id: 1});
            wiltoncall("av_setup_decoder_to_encoder", {decoder_id: 2, encoder_id: 1});
            for (var i = 0; i < 2; ++i) {
                logger.info("Server is running ...");
                thread.sleepMillis(1000);
            }
            var ph_res = wiltoncall("av_make_photo", photo_settings);
            print(ph_res); 
            // var flag = wiltoncall("av_is_started", resp);
            // print("is started: " + flag)
            // thread.sleepMillis(1000);

            wiltoncall("av_stop_video_display", display_id);
            // wiltoncall("av_stop_video_record", resp);
            // for (var i = 0; i < 2; ++i) {
            //     logger.info("Server is prepare to close ...");
            //     thread.sleepMillis(1000);
            // }
            // flag = wiltoncall("av_is_started", resp);
            // print("is started: " + flag)
            wiltoncall("av_stop_decoding", decoder_id);
            wiltoncall("av_stop_decoding", decoder_id_2);
            wiltoncall("av_stop_encoding", encoder_id);

            wiltoncall("av_delete_decoder", decoder_id);
            wiltoncall("av_delete_decoder", decoder_id_2);
            wiltoncall("av_delete_encoder", encoder_id);
            wiltoncall("av_delete_display", display_id);
            // }
            // for (var i = 0; i < 2; ++i) {
            //     logger.info("Server is closed wait ...");
            //     thread.sleepMillis(1000);
            // }
            // flag = wiltoncall("av_is_started", resp);
            // print("is started: " + flag);

            // var json = JSON.parse(flag);
            // print("is started: " + json.error);            
        }
    };
});
