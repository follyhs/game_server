#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/libuv/lib
kill -9 $(ps -ef|grep -v grep|grep ./center/center_server | awk '{print $2}')
kill -9 $(ps -ef|grep -v grep|grep ./common_server/common_server | awk '{print $2}')

kill -9 $(ps -ef|grep -v grep|grep ./game_server/game_server | awk '{print $2}')

kill -9 $(ps -ef|grep -v grep|grep ./gateway/gateway_server | awk '{print $2}')

