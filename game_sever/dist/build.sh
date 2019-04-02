#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/libuv/lib

cd ./center
rm *server
make

cd ../common_server
rm *server
make

cd ../game_server
rm *server
make

cd ../gateway
rm *server
make

