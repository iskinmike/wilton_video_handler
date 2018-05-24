C++ module wilton_video_handler
------------------

`WILTON_HOME` environment variable must be set to the Wilton dist directory.<br>
`WILTON_DIR` environment variable must be set to the Wilton source dir.<br>
For windows build: <br>
`FFMPEG_DEV_DIR` environment variable must be set to the FFMpeg dev directory, contained "/include" and "/lib" dirs.<br>
`SDL_DEV_DIR` environment variable must be set to the SDL2 dev directory, contained "/include/SDL2" and "/lib/x64/" or "/lib/x86/" dirs<br>
`STATICLIB_DIR` environment variable must be set to directory with staticlibs libraries<br>


Install libraries for Ubuntu:
```
 sudo apt install libavcodec-dev libswscale-dev libswresample-dev libavformat-dev libavutil-dev libavdevice-dev libsdl2-dev 
```

Build and run on Linux:
```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_FLAGS=--std=c++11
    make
    cd dist
    ./bin/wilton index.js
```

Build and run on Windows:
```bash
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
| av_start_video_record(**id**)  | Start record video from specified device and write it to specified file. Required handler's **id**.|
| av_start_video_display(**id**) | Create window to display video from device. Required handler's **id**.|
| av_make_photo(**id**)          | Take photo from device. Required handler's **id**. |
| av_stop_video_display(**id**)  | Stop display video from device and destroy display. Required handler's **id**. |
| av_stop_video_record(**id**)   | Stop video record and finalize file. Required handler's **id**. |
| av_delete_handler(**id**) | Release webcam and delete initialised handler. Required handler's **id** |
| av_is_started(**id**) | Returns 1 if record is started, 0 if not. Required handler's **id** |


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
  "pos_x" : 800,              // optional. Default: 100
  "pos_y" : 300               // optional. Default: 100
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


###errors handling

If video handler with **id** doesn't exists function return errror message in json string:

```JavaScript
{ 
  "error": "Wrong id [$id]"
}
``` 
