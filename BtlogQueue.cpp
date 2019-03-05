#include "log_pattern.h"
#include <sys/types.h>
#include "BtlogQueue.h"

BtlogQueue::BtlogQueue(void){
     this->index=0;
     this->log_num=0;
}
BtlogQueue::~BtlogQueue(void){}

u_int8_t BtlogQueue::enqueue(char *str){
    //判断字符串长度
    int len;
    for(len=0;len<BT_LOG_BUF_LEN;len++){
        if(*(str+len)=='\0') 
        break;
    }
    if(len == BT_LOG_BUF_LEN || len == 0){
        return 1;
    }
    if( *(str+len-1) =='\n' ){ //除去末尾的换行符(如果有)
        *(str+len-1) ='\0';
        len--;
    }
    strcpy(logs[this->index++],str);
    this->index%=BT_LOG_BUF_NUM;//循环队列

    if(log_num<BT_LOG_BUF_NUM) //已缓存的记录条数
        log_num++;

    return 0;
}

u_int8_t BtlogQueue::select(char *dst, int num){
    int select_index;
    if(num < 0 || num >= this->log_num){
        return 1;
    }
    select_index = this->index-num < 0 ? (this->index-num + BT_LOG_BUF_NUM) : (this->index-num);
    strcpy(dst, this->logs[ select_index ] );
    return 0;
}

void BtlogQueue::test(void){
    int i = this->index == 0 ? BT_LOG_BUF_NUM : this->index;            //避免i==0时查询
    int start_i = this->log_num == BT_LOG_BUF_NUM ? this->index : BT_LOG_BUF_NUM - 1;   //防越界 调通后把这里写得优雅一点

    while(--i != start_i){
        printf("%s\n",logs[i]);
        if(i==0){
            i+=BT_LOG_BUF_NUM;
        }
    }
}