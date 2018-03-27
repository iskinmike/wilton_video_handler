
#include "decoder.h"
#include "encoder.h"
#include "display.h"
#include <iostream>
#include <unistd.h>
#include "video_api.h"

using namespace std;

int main(int argc, char const *argv[])
{
    /* code */
    VideoSettings set;
    set.input_file = argv[1];
    set.output_file = argv[2];
    set.format = "video4linux2";
    set.title = "CAM VIDEO";
    set.width = 640;
    set.height = 480;
    set.pos_x = 200;
    set.pos_y = 200;
    set.bit_rate = 150000;
    set.photo_name = "test.bmp";

    VideoAPI api(set);

    api.startVideoRecord();
    api.startVideoDisplay();

    api.makePhoto();
    for (int i = 0; i <3; ++i){
        sleep(1);
        cout << "sleep: " << i << endl;
    }

    api.stopVideoDisplay();
    api.stopVideoRecord();


    return 0;
}

