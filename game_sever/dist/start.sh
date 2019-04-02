#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/libuv/lib

./center/center_server &


./common_server/common_server &


./game_server/game_server &

./gateway/gateway_server &
