INCLUDE = -I./../../pkg/hiredis/include -I./../../pkg/mysql-connector-c/include -I./../../pkg/libuv/include
center_server: \
        ../../src/center_server/main.c \
        ../../src/center_server/auth.c \
        ../../src/center_server/cener_config.c \
        ../../src/center_server/center_services.c \
        ../../src/center_server/models/login_model.c \
        \
	../../src/game_command.c \
	\
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
	../../src/database/center_db_config.c \
	../../src/database/center_database.c \
	../../src/database/game_db_config.c \
	../../src/database/game_database.c 

	gcc $(INCLUDE) -fPIC -std=c99 -g -Wall -o $@ $^ -lpthread -luv -D_GNU_SOURCE -L./../../pkg/libuv/lib -L./../../pkg/hiredis/lib -lhiredis `mysql_config --cflags --libs`
clean:
	rm center_server
