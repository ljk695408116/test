#include <stdio.h>
#include <string.h>
#include <sys/types.h>
extern "C" {
#include "regex.h"
}
#include "log_pattern.h"
#include "BtlogQueue.h"
#define _LINE_LENGTH 300

# define TEST_MODE 0

//主要保存日志信息
extern BtlogQueue logqueue;

int main(void) {
    FILE *logcat_pipe;
    char line[_LINE_LENGTH];

    int print_count=3423;
    logqueue=BtlogQueue();

    if(log_regex_init()!=0)
    {
        return 1;
    }
    printf("start!\n");

#if TEST_MODE==9
    logcat_pipe = fopen("testlog.txt", "r");//本机测试Logcat的不同格式
#else
    logcat_pipe = popen("logcat", "r");//进一步工作,测试Logcat的不同格式
#endif
    if (NULL != logcat_pipe){
	    while(1){
            while (fgets(line, _LINE_LENGTH, logcat_pipe) != NULL){
                
                if( is_btlog(line) ==0 ){
#if TEST_MODE==2
if(--print_count%10000==0) printf("running!%d\n",print_count);
#endif
                    continue;
                }
#if TEST_MODE==4
printf("%s\n", line);//测试用
#endif
                //考虑将数据缓存,可能还用不到,暂时放着
                logqueue.enqueue(line);

                //进一步提取关键日志
                if( is_keylog(line)==0 ){
#if TEST_MODE==3
if(--print_count%50==0) printf("running!%d\n",print_count);
#endif
                    continue;
                }
                log_match(line);//现在已经只剩关键日志了,需要根据不同的日志特征,报告蓝牙状态信息          
#if TEST_MODE==5
if(--print_count%25==0) logqueue.test();
#endif
            }
		}
    }
    else {
	printf("file open error!\n");//测试用
        return 1;
    }
    pclose(logcat_pipe);
    return 0;
}