C++ module wilton_video_handler
------------------

`WILTON_HOME` environment variable must be set to the Wilton dist directory.
`WILTON_DIR` environment variable must be set to the Wilton source dir.

Build and run on Linux:

    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_FLAGS=--std=c++11
    make
    cd dist
    ./bin/wilton index.js
