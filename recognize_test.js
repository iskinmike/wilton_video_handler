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
    "wilton/Socket",
    "wilton/Logger"
], function(dyload, wiltoncall, thread, Socket, Logger) {
    "use strict";

    // load shared lib on init
    dyload({
        name: "wilton_video_handler"
    });
    dyload({
        name: "wilton_net"
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

            var photo_settings = {};
            photo_settings["id"] = 1;
            photo_settings["width"] = 640;
            photo_settings["height"] = 480;
            photo_settings["photo_name"] = "photo.png";

            var recognizer_settings = {
                  id : 1, 
                  ip : "127.0.0.1", 
                  port : 7777, 
                  waitTimeMillis : 50, 
                  width : 320, 
                  height : 240, 
                  faceCascadePath : "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml", 
                  scale : 1.2, 
                  nearFacesCount : 3, 
                  minSizeX : 30, 
                  minSizeY : 30, 
                  templateDetectorMinThreshold : 0.20, 
                  matcherIntersectionPercentThreshold : 0.85, 
                  // enableDebugDisplay : 1, 
                  loggerSettings : {
                        path: "data.txt"
                  }
            };

            var decoder_id;
            var recognizer_id;
            var start_res;


        for (var counter = 0; counter < 1; ++counter) {
            decoder_id = wiltoncall("av_init_decoder", decoder_settings);
            print("av_init_decoder");
            recognizer_id = wiltoncall("av_init_recognizer", recognizer_settings);
            print("av_init_recognizer");

            wiltoncall("av_setup_decoder_to_recognizer", {decoder_id: 1, recognizer_id: 1});

            print("av_setup_decoder_to_encoder");
            start_res = wiltoncall("av_start_decoding", decoder_id);
            print("av_start_decoding: " + start_res); 

            start_res = wiltoncall("av_start_recognizer", recognizer_id);
            print("av_start_recognizer: " + start_res); 

            // create socket
            var socket = new Socket({
                ipAddress: "127.0.0.1",
                tcpPort: 7777,
                protocol: "TCP",
                role: "client",
                timeoutMillis: 500
            });
            var counter = 0;
            var exit_amount = 4;
            while (true){
                  var received = socket.read({
                      timeoutMillis: 1000
                  });
                  if (null === received) {
                        continue;
                  }
                  if (received.length){
                        print("Data: [" + received + "]");
                  }
                  counter++;
                  if (exit_amount == counter) {
                        break;
                  }
            }
           

            wiltoncall("av_stop_recognizer", recognizer_id);
            wiltoncall("av_stop_decoding", decoder_id);

            wiltoncall("av_delete_decoder", decoder_id);
            wiltoncall("av_delete_recognizer", recognizer_id);

        }    
        }
    };
});
