C++ module wilton_video_handler
------------------

`WILTON_HOME` environment variable must be set to the Wilton dist directory.<br>
For windows build: <br>
`FFMPEG_DEV_DIR` environment variable must be set to the FFMpeg dev directory, contained "/include" and "/lib" dirs.<br>
`SDL_DEV_DIR` environment variable must be set to the SDL2 dev directory, contained "/include/SDL2" and "/lib/x64/" or "/lib/x86/" dirs<br>

Install libraries for Ubuntu:
```
 sudo apt install libavcodec-dev libswscale-dev libswresample-dev libavformat-dev libavutil-dev libavdevice-dev libsdl2-dev 
```

Install libraries for CentOs7:
```
 sudo yum install ffmpeg-devel-2.6.8-3.el7.nux.x86_64 SDL2-devel-2.0.3-9.el7.x86_64
```

Module is dependent of jansson library. This library(2.11 version) included as submodule.

Build and run on Linux:
```bash
    git submodule update --init
    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_FLAGS=--std=c++11
    make
    cd dist
    ./bin/wilton index.js
```

Build and run on Windows:
```bash
    git submodule update --init
    mkdir build
    cd build
    cmake .. -G "Visual Studio 1x 201x Win64" # example "Visual Studio 14 2015 Win64"
    cmake --build . --config Release
    cd dist
    bin\wilton.exe index.js
```

Build for x32 windows system:
```bash
    cmake .. -G "Visual Studio 1x 201x"
    cmake --build . --config Release
```

## warning
If you use Visual Studio different from VS2013, you should install vcredist 20xx, depending on your VS version.

### usage
| functions| Description |
| --- | --- |
| av_inti_handler(**settings**)  | Initialize video handler with specified **settings**. Return handler **id** as *string* value. |
| av_delete_handler(**id**) | Release webcam and delete initialised handler. Required handler's **id** |
| av_start_video_record(**id**)  | Start record video from specified device and write it to specified file. Required handler's **id**. This Call includes call for start decoder and encoder |
| av_stop_video_record(**id**)   | Stop video record and finalize file. Required handler's **id**. |
| av_make_photo(**id**)          | Take photo from device. Required handler's **id**. |
| av_start_video_display(**id**) | Create window to display video from device. Required handler's **id**.|
| av_stop_video_display(**id**)  | Stop display video from device and destroy display. Required handler's **id**. |
| av_start_decoding(**id**) | Start Decoder that occupies a video device and start to decode frames to memory. Required handler's **id** |
| av_stop_decoding(**id**) | Stop Decoder and release video device. Required handler's **id** |
| av_start_encoding(**id**) | Start Encoder, that wait for decoded frames and writes them to video file. Required handler's **id** |
| av_stop_encoding(**id**) | Stop Encoder. Required handler's **id** |
| av_is_started(**id**) | Returns 1 if record is started, 0 if not. Required handler's **id** |
| av_is_decoder_started(**id**) | Returns 1 if decoder is started, 0 if not. Required handler's **id** |
| av_is_encoder_started(**id**) | Returns 1 if encoder is started, 0 if not. Required handler's **id** |
| av_is_display_started(**id**) | Returns 1 if display of frames is started, 0 if not. Required handler's **id** |
| av_start_recognizer(**id**) | Start recognizing algorithm. Required handler's **id** |
| av_stop_recognizer(**id**) | Stop recognizing algorithm. Required handler's **id** |
| av_is_recognizing_in_progress(**id**) | Returns 1 if recognizer is started, 0 if not. Required handler's **id** |


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
  "bit_rate" : 150000,        // optional. Default: device bit_rate
  "framerate" : 4.4,          // optional, should be setted as float value. Default: 25.0 
  // pixel pos of left top corner of created display
  "pos_x" : 800,              // optional. Position X of created window. Default: 100
  "pos_y" : 300               // optional. Position Y of created window. Default: 100
// face detection
  "recognizer_ip" : "127.0.0.1" // Required. Ip of server that will send alerts if detected faces not equal to 1. 
  "recognizer_port" : 7777      // Required. Port of server.
  "wait_time_ms" :  500         // Required. Time of delay between matching frames.
                                // Required. Patth to cascade mesh. On linux this i standard path if tou setup opencv.
  "face_cascade_path" : "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"; 
}


```


Changes for windows:
```JavaScript
{
  "in" : "video=HP Webcam"; // Name of input device. required
  "fmt" : "dshow";          // Name of input ffmpeg format. required
}
```

 Call function from JS example:
```JavaScript
  var settings = {};
  settings["id"] = 1;
  settings["framerate"] = 4.4;
  ... // do settings stuff
  var int_id = 1;
  var str_id = wiltoncall("av_inti_handler", settings);
  wiltoncall("av_start_video_record", int_id); // no matter int or string
  ... // do something
  wiltoncall("av_stop_video_record", str_id);
```


A more detailed example in **index.js**


### errors handling

Errors returned as stringify JSON with error property.

For example, if video handler with **id** doesn't exists function return errror message in json string:

```JavaScript
{ 
  "error": "Wrong id [$id]"
}
``` 


### Face detection

Module provides server for sending errors and face detection algorithm. Server starts when video handler created by call av_inti_handler().
If it detects number of faces different from 1 it send alert message to client. In case of error send error message;

alert message:
```JavaScript
{
  "alert_number" : ##
  "faces_count" : ##
}
```

error message:
```JavaScript
{
  "error" : "error text"
}
```

usage example with wilton_net sockets:

```JavaScript
dyload({
    name: "wilton_video_handler"
});
dyload({
    name: "wilton_net"
});

...

var settings = {};
settings["id"] = 1;
...
settings["recognizer_ip"] = "127.0.0.1";
settings["recognizer_port"] = 7777;
settings["wait_time_ms"] = 1;
settings["face_cascade_path"] = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"; 

...

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
for (var i = 0; i < 20; ++i) {
    thread.sleepMillis(1000);
    var read_conf = {};
    read_conf["socketHandle"] = JSON.parse(socket_handler).socketHandle;
    read_conf["timeoutMillis"] = 1000;
    var data = wiltoncall("net_socket_read", read_conf);
    print("=== data: [" + data +"]")
}
wiltoncall("av_stop_recognizer", resp);
print("=== av_stop_recognizer");

wiltoncall("net_socket_close", JSON.parse(socket_handler));
```