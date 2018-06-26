/* 
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
    dyload({
        name: "wilton_net"
    });
    var logger = new Logger("server.main");
    
    return {
        main: function() {
            Logger.initConsole("INFO");
            print("Calling native module ...");
            var settings = {};
            settings["id"] = 1;
            settings["in"] = "/dev/video0";   // linux
            settings["fmt"] = "video4linux2"; // linux
            // settings["in"] = "video=HP Webcam";  // windows
            // settings["fmt"] = "dshow";           // windows
            settings["out"] = "out.mp4";
            settings["title"] = "CAM";
            settings["width"] = 640;
            settings["height"] = 480;
            settings["pos_x"] = 800;
            settings["pos_y"] = 300;
            settings["bit_rate"] = 150000;
            settings["photo_name"] = "photo.png";
            settings["framerate"] = parseFloat("10");
            settings["recognizer_ip"] = "127.0.0.1";
            settings["recognizer_port"] = 7777;
            settings["wait_time_ms"] = 1;
            settings["face_cascade_path"] = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"; //haarcascade_profileface.xml

            var resp = wiltoncall("av_inti_handler", settings);
            wiltoncall("av_start_video_record", 1);
            var flag = wiltoncall("av_is_started", resp);
            thread.sleepMillis(1000);

            var socket = {}
            socket["ipAddress"] = "127.0.0.1";
            socket["tcpPort"] = 7777;
            socket["protocol"] = "TCP";
            socket["role"] = "client";
            socket["timeoutMillis"] = 2000;
            var socket_handler = wiltoncall("net_socket_open", socket);
            print("=== socket_handler: " + socket_handler);
            
            wiltoncall("av_start_recognizer", resp);
            print("=== av_start_recognizer");
            for (var i = 0; i < 60; ++i) {
                thread.sleepMillis(1000);
                var read_conf = {};
                read_conf["socketHandle"] = JSON.parse(socket_handler).socketHandle;
                read_conf["timeoutMillis"] = 1000;
                var data = wiltoncall("net_socket_read", read_conf);
                print("=== data: [" + data +"]")
            }
            wiltoncall("av_stop_recognizer", resp);
            print("=== av_stop_recognizer");

            thread.sleepMillis(1000);

            wiltoncall("net_socket_close", JSON.parse(socket_handler));

            wiltoncall("av_stop_video_record", resp);
            wiltoncall("av_delete_handler", resp);
        }
    };
});