#include <stdio.h>
#include <stdlib.h>
#include "include/uv.h"

int main(char argc, char *argv[])
{
	    uv_loop_t *loop = uv_loop_new();  // 可以理解为新建一个消息队列
	        uv_run(loop, UV_RUN_DEFAULT);     // 启动消息队列，UV_RUN_DEFAULT模式下，当消息数为0时，就会退出消息循环。
		    printf("hello, world\n");
		        return 0;
}
