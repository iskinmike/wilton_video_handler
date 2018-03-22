/*
 * Copyright 2018, alex at staticlibs.net
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
            var settings = {};
            settings["id"] = 1;
            settings["in"] = "/dev/video0";
            settings["out"] = "out.avi";
            settings["fmt"] = "video4linux2";
            settings["title"] = "CAM";
            // settings["width"] = 480;
            // settings["height"] = 320;
            // settings["pos_x"] = 800;
            // settings["pos_y"] = 300;
            // settings["bit_rate"] = 147456000;
            settings["photo_name"] = "test.bmp";


            var resp = wiltoncall("intiHandler", settings);
            // var display_settings = {};
            // display_settings["id"] = resp;
            // display_settings["x"] = 200;
            // display_settings["y"] = 200;
            // print("Call response: [" + resp + "]");

            wiltoncall("startVideoRecord", resp);
            wiltoncall("displayVideo", resp);
            for (var i = 0; i < 2; ++i) {
                logger.info("Server is running ...");
                thread.sleepMillis(1000);
            }

            // var photo_settings = {};
            // photo_settings["id"] = resp;
            // photo_settings["out"] = "photo1.bmp";

            wiltoncall("makePhoto", resp);
            thread.sleepMillis(1000);

            wiltoncall("stopDisplayVideo", resp);
            wiltoncall("stopVideoRecord", resp);
        }
    };
});
