C++ module wilton_video_handler
------------------
For all builds:<br>
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
 sudo yum install ffmpeg-devel.x86_64 SDL2-devel.x86_64
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
    cmake .. -G "Visual Studio 12 2013 Win64" # example "Visual Studio 14 2015 Win64"
    cmake --build . --config Release
    cd dist
    bin\wilton.exe index.js
```

Build for x32 windows system:
```bash
    cmake .. -G "Visual Studio 1x 201x" -DARCHITECTURE=x86
    cmake --build . --config Release
```

## warning
If you use Visual Studio different from VS2013, you should install vcredist 20xx, depending on your VS version.

### usage
| functions| Description |
| --- | --- |
| av_init_decoder(**decoder-settings**) | Initialize decoder with specified **decoder-settings**. Return handler **id** as *string* value. |
| av_init_encoder(**encoder-settings**) | Initialize encoder with specified **encoder-settings**. Return handler **id** as *string* value. |
| av_init_display(**display-settings**) | Initialize display with specified **display-settings**. Return handler **id** as *string* value. |
| av_init_recognizer(**recognizer-settings**) | Initialize recognizer with specified **recognizer-settings**. Return handler **id** as *string* value. |
| av_delete_decoder(**id**) | Delete initialized decoder. Required decoder **id** |
| av_delete_encoder(**id**) | Delete initialized encoder. Required encoder **id** |
| av_delete_display(**id**) | Delete initialized display. Required display **id** |
| av_delete_recognizer(**id**) | Delete initialized recognizer. Required display **id** |
| av_setup_decoder_to_display(**settings**) | Setup decoder to display. Display takes frames from decoder. Required decoder **id** and display **id**. |
| av_setup_decoder_to_encoder(**settings**) | Setup decoder to encoder. Encoder takes frames from decoder. Required decoder **id** and recognizer **id**. |
| av_setup_decoder_to_recognizer(**settings**) | Setup decoder to recognizer. Encoder takes frames from decoder. Required decoder **id** and encoder **id**. |
| av_make_photo(**photo-settings**) | Take photo from specified decoder. Required decoder **id**. |
| av_start_video_display(**id**) | Create window to display video from device. Required display **id**.|
| av_stop_video_display(**id**)  | Stop display video from device and destroy display. Required display **id**. |
| av_start_decoding(**id**) | Start Decoder that occupies a video device and start to decode frames to memory. Required decoder **id** |
| av_stop_decoding(**id**) | Stop Decoder and release video device. Required decoder **id** |
| av_start_encoding(**id**) | Start Encoder, that wait for decoded frames and writes them to video file. Required encoder **id** |
| av_stop_encoding(**id**) | Stop Encoder. Required encoder **id** |
| av_is_decoder_started(**id**) | Returns 1 if decoder is started, 0 if not. Required handler's **id** |
| av_is_encoder_started(**id**) | Returns 1 if encoder is started, 0 if not. Required handler's **id** |
| av_is_display_started(**id**) | Returns 1 if display of frames is started, 0 if not. Required handler's **id** |
| av_start_recognizer(**id**) | Start recognizing algorithm. Required handler's **id** |
| av_stop_recognizer(**id**) | Stop recognizing algorithm. Required handler's **id** |
| av_is_recognizing_in_progress(**id**) | Returns 1 if recognizer is started, 0 if not. Required handler's **id** |


Settings 
decoder_settings: 
```JavaScript
{
  "id" : 1,                   // You may setup id manually. Optional. Default value 0;
  "in" : "/dev/video0",       // Name of input device. Required
  "fmt" : "video4linux2",     // Name of input ffmpeg format. Required
  // time base determined as numerator/denumerator.
  "time_base_den" : 1000000   // Optional. Setup camera time base denumerator. Standart value for Linux: 1000000
  "time_base_num" : 1         // Optional. Setup camera time base numerator. Standart value: 1
}
```
encoder_settings: 
```JavaScript
{
  "id" : 1,                   // You may setup id manually. Optional. Default value 0;
  "out" : "out.mp4",          // Name of output file. Required  
  "width" = 480;              // Width of video file. Optional. Default: width of captured device image
  "height" = 320;             // Height of video file. Optional. Default: height of captured device image
  "bit_rate" : 150000,        // Optional. Default: device bitrate
  "framerate" : 4.4,          // Optional, should be setted as float value. Default: 25.0 
}
```
display_settings: 
```JavaScript
{
  "id" : 1,                   // You may setup id manually. Optional. Default value 0;
  "title" : "CAM",            // Name of created display. Required           
  "parent_title" : "Firefox", // Name of runned process window to append created display. Optional           
  "width" = 320;              // Width of display file. Optional. Default: width of captured device image
  "height" = 400;             // Height of display file. Optional. Default: height of captured device image
  // pixel pos of left top corner of created display
  "pos_x" : 300,              // Optional. Position X of created window relative to connected parent window. Default: 100
  "pos_y" : 300               // Optional. Position Y of created window relative to connected parent window. Default: 100
}
```
photo_settings: 
```JavaScript
{
  "id" : 1,                   // Decoder **id**. Required
  "photo_name" : "photo.png", // Name of created photo. Required
  "width" = 128;              // Width of photo file. Optional. Default: width of captured device image
  "height" = 128;             // Height of photo file. Optional. Default: height of captured device image
}
```
recognizer_settings: 
```JavaScript
{
  "id" : 1,                   // Decoder **id**. Required
  "width" = 128;              // Width of photo file. Optional. Default: width of captured device image
  "height" = 128;             // Height of photo file. Optional. Default: height of captured device image
// face detection
  "ip" : "127.0.0.1" // Required. Ip of server that will send alerts if detected faces not equal to 1. 
  "port" : 7777      // Required. Port of server.
  "waitTimeMillis" :  500         // Required. Time of delay between matching frames.
                                // Required. Path to cascade mesh. On linux this is standard path if tou setup opencv.
  "faceCascadePath" : "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"; 
  // Cascade detection options. More info: https://docs.opencv.org/2.4/modules/objdetect/doc/cascade_classification.html#cascadeclassifier-detectmultiscale
  "scale" : 1.2           // scaleFactor – Parameter specifying how much the image size is reduced at each image scale.
  "nearFacesCount" : 3    // minNeighbors – Parameter specifying how many neighbors each candidate rectangle should have to retain it.
  "minSizeX" : 30         // minSize – Minimum possible object size. Objects smaller than that are ignored. X axis value
  "minSizeY" : 30         // minSize – Minimum possible object size. Objects smaller than that are ignored. Y axis value
  // Templates detection options. More info below.
  "templateDetectorMinThreshold" : 0.20           // Minimal threshold for matcher function if min value is above this threshold mark template as detected
  "matcherIntersectionPercentThreshold" : 0.85    // determines intersection between new finded face and face search area of already created template. 
                                                  // If intersection is above this value we delete one of intesected templates.
  // Debug otions
  "enableDebugDisplay" : 1  // Creates display with detected templates. Purple circle - face finded by cascade. Red square - tracked template.
  "loggerSettings" : {   
    "path" : "data.txt"     // Saves some debug information into specified file.
  }
}
```

 - With some cameras there is a trouble with time_base determination. You may setup time_base manually.
On windows HP cam you may try options 
```js
{"time_base_den" : 10000000, 
  "time_base_num" : 1}
```

Changes for windows:
```JavaScript
{
  "in" : "video=HP Webcam"; // Name of input device. Required
  "fmt" : "dshow";          // Name of input ffmpeg format. Required
}
```
Camera name may be found at windows device manager or through ffmpeg.

A more detailed example in **index.js**


### errors handling

Errors returned as stringify JSON with error property.

For example, if display, decoder or encoder handler with **id** doesn't exists function return errror message in json string:

```JavaScript
{ 
  "error": "Wrong display/decoder/encoder id [$id]"
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