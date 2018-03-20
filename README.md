C++ module example
------------------

`WILTON_HOME` environment variable must be set to the Wilton dist directory.

Build and run on Windows:

    mkdir build
    cd build
    cmake .. -G "Visual Studio 1x 20xx Win64"
    cmake --build .
    cd dist
    bin\wilton.exe index.js

Build and run on Linux:

    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_FLAGS=--std=c++11
    make
    cd dist
    ./bin/wilton index.js
