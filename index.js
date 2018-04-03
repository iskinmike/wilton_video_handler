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
            settings["out"] = "out.mp4";
            settings["fmt"] = "video4linux2";
            settings["title"] = "CAM";
            settings["width"] = 640;
            settings["height"] = 480;
            settings["pos_x"] = 800;
            settings["pos_y"] = 300;
            settings["bit_rate"] = 150000;
            settings["photo_name"] = "photo.png";

            var resp = wiltoncall("av_inti_handler", settings);
            wiltoncall("av_start_video_record", 1);
            wiltoncall("av_start_video_display", resp);
            for (var i = 0; i < 2; ++i) {
                logger.info("Server is running ...");
                thread.sleepMillis(1000);
            }
            wiltoncall("av_make_photo", resp);
            thread.sleepMillis(1000);

            wiltoncall("av_stop_video_display", resp);
            wiltoncall("av_stop_video_record", resp);
        }
    };
});
