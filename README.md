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
| av_delete_decoder(**id**) | Delete initialised decoder. Required decoder **id** |
| av_delete_encoder(**id**) | Delete initialised encoder. Required encoder **id** |
| av_delete_display(**id**) | Delete initialised display. Required display **id** |
| av_setup_decoder_to_display(**settings**) | Setup decoder to display. Display takes frames from decoder. Required decoder **id** and display **id**. |
| av_setup_decoder_to_encoder(**settings**) | Setup decoder to encoder. Encoder takes frames from decoder. Required decoder **id** and encoder **id**. |
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


Settings 
decoder_settings: 
```JavaScript
{
  "id" : 1,                   // You may setup id manually. Optional. Default value 0;
  "in" : "/dev/video0",       // Name of input device. Required
  "fmt" : "video4linux2",     // Name of input ffmpeg format. Required
  "framerate": "60/1",        // Optiopnal. Encoded stream framerate.
  "videoformat": "mjpeg",     // Optiopnal. Encoded stream video format.
  "size": "1280x720",         // Optiopnal.  Encoded stream size.
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
