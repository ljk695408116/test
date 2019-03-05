#ifndef _BTlogQueue_H
#define _BTlogQueue_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

class BtlogQueue{

    private:
        static const int BT_LOG_BUF_NUM=20;
        static const int BT_LOG_BUF_LEN=200;      
        int index;
        int log_num;
        char logs[BT_LOG_BUF_NUM][BT_LOG_BUF_LEN];

    public:

        BtlogQueue();
        ~BtlogQueue();
        u_int8_t enqueue(char *str);
        u_int8_t select(char *dst, int num);
        void test(void);

};



#endif