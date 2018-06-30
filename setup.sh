#!/bin/sh

 mkdir external && cd external
 git submodule add -f https://github.com/gabime/spdlog.git
 git submodule update --init
 cd -
 mkdir build
 echo "ready to build:"
 echo "   cd build && cmake .. -DCMAKE_BUILD_TYPE=release [-DVCTCXO=on] && make"
 
