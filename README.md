C++ module wilton_video_handler
------------------

`WILTON_HOME` environment variable must be set to the Wilton dist directory.<br>
`WILTON_DIR` environment variable must be set to the Wilton source dir.<br>
For windows build: <br>
`FFMPEG_DEV_DIR` environment variable must be set to the FFMpeg dev directory, contained "/include" and "/lib" dirs.<br>
`SDL_DEV_DIR` environment variable must be set to the SDL2 dev directory, contained "/include/SDL2" and "/lib/x64/" dirs<br>

Build and run on Linux:

    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_FLAGS=--std=c++11
    make
    cd dist
    ./bin/wilton index.js



### usage
| functions| Description |
| --- | --- |
| av_inti_handler(**settings**)  | Initialize video handler with specified **settings**. Return handler **id** as *string* value. |
| av_start_video_record(**id**)  | Start record video from specified device and write it to specified file. Required handler's **id**.|
| av_start_video_display(**id**) | Create window to display video from device. Required handler's **id**.|
| av_make_photo(**id**)          | Take photo from device. Required handler's **id**. |
| av_stop_video_display(**id**)  | Stop display video from device and destroy display. Required handler's **id**. |
| av_stop_video_record(**id**)   | Stop video record and finalize file. Required handler's **id**. |

Settings json: 
```JavaScript
{
  "id" : 1,                   // You may setup id manually. optional. Default value 0;
  "in" : "/dev/video0",       // Name of input device. required
  "out" : "out.mp4",          // Name of output file. required  
  "fmt" : "video4linux2",     // Name of input ffmpeg format. required
  "photo_name" : "photo.png", // Name of created photo. required
  "title" : "CAM",            // Name of created display. required
  "width" : 640,              // optional. Default: width of captured device image
  "height" : 480,             // optional. Default: height of captured device image
  "bit_rate" : 150000,        // oprional. Default: device bit_rate
  // pixel pos of left top corner of created display
  "pos_x" : 800,              // optional. Default: 100
  "pos_y" : 300               // optional. Default: 100
}
```

 Call function from JS example:
```JavaScript
  var settings = {};
  settings["id"] = 1;
  ... // do settings stuff
  var int_id = 1;
  var str_id = wiltoncall("av_inti_handler", settings);
  wiltoncall("av_start_video_record", int_id); // no matter int or string
  ... // do something
  wiltoncall("av_stop_video_record", str_id);
```


A more detailed example in **index.js**