INCLUDE = -I./../../pkg/hiredis/include -I./../../pkg/mysql-connector-c/include
common_server: \
        ../../src/common_server/main.c \
        ../../src/common_server/common_config.c \
        ../../src/common_server/common_services.c \
        ../../src/common_server/login_bonueses.c \
        ../../src/common_server/mask_and_bonues.c \
        ../../src/common_server/ugame_common.c \
        ../../src/common_server/models/model_login_bonues.c \
        ../../src/common_server/models/model_mask_and_bonues.c \
        ../../src/common_server/models/model_ugame_common.c \
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

	gcc $(INCLUDE) -std=c99 -g -Wall -o $@ $^ -lpthread -luv -D_GNU_SOURCE -L./../../pkg/libuv/lib/ -L./../../pkg/hiredis/lib -lhiredis `mysql_config --cflags --libs`
clean:
	rm common_server
