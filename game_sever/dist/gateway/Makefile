INCLUDE = -I./../../pkg/hiredis/include -I./../../pkg/mysql-connector-c/include -I./../../pkg/libuv/include
gateway_server: ../../src/gateway_server/main.c \
	../../src/gateway_server/from_client.c  \
        ../../src/gateway_server/gw_config.c \
        ../../src/gateway_server/server_post.c \
        ../../src/gateway_server/server_return.c \
        ../../src/gateway_server/session_key_man.c \
        ../../src/gateway_server/server_session.c \
        ../../src/3rd/crypt/base64_decode.c \
	../../src/3rd/crypt/base64_encoder.c  \
        ../../src/3rd/crypt/sha1.c \
        ../../src/3rd/md5/md5.c \
        ../../src/3rd/mjson/json.c \
        ../../src/3rd/mjson/json_extends.c \
        ../../src/3rd/http_parser/http_parser.c \
	../../src/utils/cache_allocer.c \
	../../src/utils/hash_int_map.c \
	../../src/utils/log.c \
	../../src/utils/random.c \
	../../src/utils/timestamp.c \
	../../src/utils/vector_array.c \
	../../src/utils/timer_list.c \
        \
	../../src/netbus/netbus.c \
	../../src/netbus/socket/session.c \
	../../src/netbus/socket/server_uv.c \
	\
	../../src/game_command.c \
	../../src/database/center_db_config.c \
	../../src/database/center_database.c \
	../../src/database/game_db_config.c \
	../../src/database/game_database.c 

	gcc $(INCLUDE) -std=c99 -g -Wall -o $@ $^ -lpthread -luv -D_GNU_SOURCE -L./../../pkg/libuv/lib/ -L./../../pkg/hiredis/lib -lhiredis `mysql_config --cflags --libs`

clean:
	rm gateway_server
