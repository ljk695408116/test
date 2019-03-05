#include "log_pattern.h"
#include <stdio.h>
#include <string.h>

extern "C" {
#include "regex.h"
}
#include "BtlogQueue.h"

static const char log_pattern[][40]={
"onBluetoothServiceUp",         //0:车机开启蓝牙服务
"onBluetoothServiceDown",       //1:车机关闭蓝牙服务
//配对过程不显示在协议栈中
"evt: SECURITY_COMPLETE",       //2:l2cap秘钥配对,作为连接中,秘钥配对的标志
"btm_sec_disconnect",  //3:暂且认为是连接取消标志(先不管)   

//bta_dm_pm_cback: new conn_srvc id:27, app_id:1 count:5
"cback: new conn_srvc id:27, app_id:1",//4认为是hands free profile服务启动的标志

//02-26 19:47:40.541 27738 27777 E bt_stack: [ERROR:bta_hf_client_rfc.cc(131)]
//bta_hf_client_mgmt_cback: closing port handle 24dev 90:2b:d2:27:4d:a4
"bta_hf_client_mgmt_cback: closing", //5 暂且认为是hand free profile 服务结束的标志 可记录地址

//02-26 14:44:05.862 27738 27738 D A2dpMediaBrowserService: 
//handleConnectionStateChange: newState=0 btDev=2C:57:31:38:F1:FB
"ce: handleConnectionStateCh", //6 audio/video profile 的 开启/关闭由newstate的值确定

//02-28 14:24:20.222  2515  2562 W bt_btif : bta_av_disconnect_req: conn_lcb: 0x1 peer_addr: 2c:57:31:38:f1:fb
"bta_av_disconnect_req", 
};

#define KEYLOG_NUM ( sizeof(log_pattern)/sizeof(log_pattern[0]) )

static regex_t each_key_reg[KEYLOG_NUM];//关键日志正则
static regex_t bt_reg,keylog_reg,time_reg,address_reg;

static char time_pattern[]="[0-1][0-9]-[0-3][0-9] ([0-9]{2}:){2}[0-9]{2}.[0-9]{3}"; //时间的正则表达式
static char address_pattern[]="([0-9a-zA-Z]{2}:){5}[0-9a-zA-Z]{2}";                 //地址的正则表达式

BtlogQueue logqueue;

//筛选bt,可改为子串搜索
static u_int8_t bt_comp(regex_t *reg){
    int errno,cflags=REG_EXTENDED;
    char ebuf[128];//存放错误日志
    errno = regcomp(reg,"bt", cflags);
    if (errno != 0){
        regerror(errno, reg, ebuf, sizeof(ebuf));
        fprintf(stderr, "%s: compile bt error \n",ebuf);
        return 1;
    }
    return 0;
}
//编译地址正则表达式
static u_int8_t address_comp(regex_t *reg){
    int errno,cflags=REG_EXTENDED;
    char ebuf[128];//存放错误日志
    errno = regcomp(reg,address_pattern, cflags);
    if (errno != 0){
        regerror(errno, reg, ebuf, sizeof(ebuf));
        fprintf(stderr, "%s: compile time error \n",ebuf);
        return 1;
    }
    return 0;
}

//编译筛选时间的正则,但说实话,如果日志格式固定,那么并不需要这个正则
//先留着,到时候看看要不要用
static u_int8_t time_comp(regex_t *reg){
    int errno,cflags=REG_EXTENDED;
    char ebuf[128];//存放错误日志
    errno = regcomp(reg,time_pattern, cflags);
    if (errno != 0){
        regerror(errno, reg, ebuf, sizeof(ebuf));
        fprintf(stderr, "%s: compile time error \n",ebuf);
        return 1;
    }
    return 0;
}

//构造关键日志的正则表达式
static u_int8_t keylog_comp(regex_t *cmb_reg){
    int errno,cflags=REG_EXTENDED;
    char ebuf[128];                     //存放错误日志
    char pattern[sizeof(log_pattern)];  //长度应该够了
    int i;
    //关键段落构造pattern
    pattern[0]='\0';
    for(i=0;i<KEYLOG_NUM;i++){
        strcat(pattern,"(");
        strcat(pattern,log_pattern[i]);
        strcat(pattern,")");
        strcat(pattern,"|");
    }
    int len=strlen(pattern);
    if(len==0){
        printf("no pattern!\n");
    }
    pattern[len-1]='\0';//补字符串结尾
    printf("%s\n",pattern);

    errno = regcomp(cmb_reg,pattern, cflags);
    if (errno != 0){
        regerror(errno, cmb_reg, ebuf, sizeof(ebuf));
        fprintf(stderr, "%s: pattern 'address' \n",ebuf);
        return 1;
    }
    return 0;
}

static u_int8_t each_keylog_comp(regex_t key_reg[]){
    int errno,cflags=REG_EXTENDED;
    char ebuf[128];     //存放错误日志
    int i;
    //构造每一条关键日志的正则表达式,如打算采用子串搜索的方式,则这部分删掉
    for(i=0;i<KEYLOG_NUM;i++){
        errno = regcomp(&key_reg[i],log_pattern[i], cflags);
        if (errno != 0){
            regerror(errno, &key_reg[i], ebuf, sizeof(ebuf));
            fprintf(stderr, "%s: pattern %s \n",ebuf,log_pattern[i]);
            return 1;
        }   
    }
    return 0;
}
//单条日志匹配
static u_int8_t regex_match(regex_t *bt_reg , char *str){
    int len,errno;
    size_t nmatch=1;
    regmatch_t matchptr[1];
    if ((len = strlen(str)) > 0 && str[len-1] == '\n')    //处理输入数据,将换行变成字符串结尾
        str[len - 1] = 0;
    errno = regexec(bt_reg, str, nmatch, matchptr, 0);
    if(errno==REG_NOMATCH){
        return 0;   //无异常
    }
    else if(errno==REG_ESPACE){
        return 2;   
    }
    else{
        return 1;
    }
}

//单条日志匹配,重载 返回偏移值,引用传参
static u_int8_t regex_match(regex_t *bt_reg , char *str,regmatch_t &matchptr ){
    int len,errno;
    size_t nmatch=1;
    if ((len = strlen(str)) > 0 && str[len-1] == '\n')    //处理输入数据,将换行变成字符串结尾
        str[len - 1] = 0;
    errno = regexec(bt_reg, str, nmatch, &matchptr, 0);
    if(errno==REG_NOMATCH){
        return 0;   //无异常
    }
    else if(errno==REG_ESPACE){
        return 2;   
    }
    else{
        return 1;
    }
}

//获得当前时间字符串
static u_int8_t get_time(char *str,char *time){
    if( regex_match(&time_reg ,str)==0){
        printf("time error:\n%s\n",str);
        return 1;
    } //不匹配
    memcpy(time,str+0,18);
    time[18]='\0';//补一个字符串结尾
    return 0;
}
//获得当前时间字符串,重载以提供读秒功能
static u_int8_t get_time(char *str,char *time,int &second){
    if( regex_match(&time_reg ,str)==0){
        printf("time error:\n%s\n",str);
        return 1;
    } //不匹配
    memcpy(time,str+0,18);
    time[18]='\0';//补一个字符串结尾
    second=(time[12]-'0')*10+(time[13]-'0');
    return 0;
}


//获得当前时间字符串,重载以提供读毫秒功能.不好写,不需要,算了吧
// static u_int8_t get_time(char *str,char *time,int msecond){
//     if( regex_match(&time_reg ,str)==0){
//         return 1;
//     } //不匹配
//     memcpy(time,str+0,18);
//     time[18]='\0';//补一个字符串结尾
//     msecond=(time[15]-'0')*100+(time[16]-'0')*10+(time[17]-'0');
//     return 0;
// }

//从字符串中提取address
static u_int8_t get_address(char *str,char *address){
    regmatch_t matchptr;
    int len;
    if( regex_match(&address_reg ,str,matchptr)==0){
        printf("address don't match\n%s\n",str);
        return 1;
    } //不匹配

    //提取地址信息
    len=matchptr.rm_eo-matchptr.rm_so;
    memcpy(address,&str[matchptr.rm_so],len);
    address[len]='\0';
    return 0;
}

//acl连接建立
static u_int8_t acl_conn_info(char *str){
    char s[200];
    char address[20];
    char time_now_s[20],time_log_s[20];
    int  time_now,time_log;

    //记录当前时间
    if( get_time(str,time_now_s,time_now) != 0){
        return 3;
    }

    for(int i = 2 ; i < 18; i++ ){     //类的设计问题 i==1指str自身
        logqueue.select(s,i);          //提取缓存的日志

        if( strstr(s,log_pattern[2]) != NULL){
            break;
        }

        if( strstr(s,"remote version info") == NULL){
            continue;
        }

        //提取日志时间
        if( get_time(str,time_log_s,time_log) != 0){    
            return 3;
        }   

        //判断日志时间是否匹配 相差超过一秒就算不匹配
        if( time_now-time_log>1  && time_log-time_now < 59 ){
            printf("err1\n   ");
            break;
        }

        if( get_address(s,address) != 0){               //提取地址
            logqueue.select(s,i); return 2;
        }

        //另一条日志佐证
        int j;
        for(j = i ; j < 18; j++ ){
            logqueue.select(s,j); 

            if( strstr(s,"new acl connetion:count") != NULL){
                break;
            }
        }

        if( j == 18){
            return 1;
        }

        //作相应处理,比如提取地址信息
        printf("a device connected   ");
        printf("address: %s\n",address);
        return 0;
    }
    //printf("don't match\n");
    return 2;

    
}

//acl连接断开
static u_int8_t acl_disconn_info(char *str){
    printf("a device disconnected~\n");
    return 0;
}
            

//免提关闭信息
//该日志包含地址,意义明确(2019.3.1),旧车机
static u_int8_t get_hf_on_info(char *str){
    char time[20],address[20];

    if( get_time(str,time) != 0){
        return 3;
    }

    if( get_address(str,address) != 0){
        return 2;
    }

    printf("%s:a device stop hands free service\n",time);

    printf("address:%s\n",address);

    return 0;
}

//av打开/关闭
static u_int8_t get_av_info(char *str){
    char *index,time[20],address[20];
    static char str_state[]="newState=";
    int offset=sizeof(str_state)-1;//获取偏移,定位状态

    if( get_time(str,time) != 0){
        return 3;
    }
    if( get_address(str,address) != 0){
        return 2;
    }

    //判断状态的正则 改成用子串
    index=strstr(str,"newState=");
    if(index == NULL){
        printf("state don't match\n%s\n",str);
        return 1;
    }
    
    if(*(index+offset)=='2'){
        printf("%s:a device start audio/vedio service\n",time);
    }else if (*(index+offset)=='0'){
        printf("%s:a device stops audio/vedio service\n",time);
    }else{
        printf("state=%c\n",*(index+offset));
        return 4;
    }
    
    printf("address: %s\n",address);

    return 0;
}

//av关闭(车机主动关闭)
static u_int8_t get_av_down_info(char *str){
    char address[20],time[20];

    if( get_time(str,time) != 0){
        return 3;
    }

    if( get_address(str,address) != 0){
        return 2;
    }
    printf("%s:a device stops audio/vedio service\n",time);
    printf("address: %s\n",address);

    return 0;
}

//匹配了蓝牙启动
static u_int8_t bt_up_info(char *str){
    char time[20];
    if( get_time(str,time) != 0){
        printf("time error:\n%s\n",str);
        return 3;
    }
    printf("%s:bt_up!\n",time);
    return 0;
}

//匹配了蓝牙关闭
static u_int8_t bt_down_info(char *str){
    char time[20];
    if( get_time(str,time) != 0){
        printf("time error:\n%s\n",str);
        return 3;
    }
    printf("%s:bt_down!\n",time);
    return 0;
}
//匹配了免提开启
static u_int8_t hf_on_info(char *str){
    char time[20];
    if( get_time(str,time) != 0){
        printf("time error:\n%s\n",str);
        return 3;
    }
    printf("%s:a device start hands free service\n",time);
    return 0;
}
//根据正则表达式(或子串),确认关键日志的具体信息
static u_int8_t singal_log_match(char *str){
    int i;
    for(i=0;i<KEYLOG_NUM;i++){
        if( strstr(str,log_pattern[i]) == NULL ){
            continue;
        }
        break;//匹配这一行
    }
    switch (i){
        case 0://数字用define起个名吧
            bt_up_info(str);
            break;
        case 1:
            bt_down_info(str);
            break;
        case 2:
            acl_conn_info(str);
            break;
        case 3:
            acl_disconn_info(str);
            break;
        case 4:
            hf_on_info(str);
            break;
        case 5:
            get_hf_on_info(str);
            break;
        case 6:
            get_av_info(str);
            break;
        case 7:
            get_av_down_info(str);
            break;

        default : 
         printf("a log doesn't match!\n");//测试用
         printf("%s\n",str);//测试用
    }
}

//编译所有相关的正则表达式
u_int8_t log_regex_init(void){
    if(bt_comp(&bt_reg)!=0){
        printf("bt_comp error!\n");//测试用
        return 1;
    }

    if(keylog_comp(&keylog_reg)!=0){
        printf("keylog_comp error!\n");//测试用
        return 1;
    }

    //这部分用子串搜索
    // if(each_keylog_comp(each_key_reg)!=0) {
    //     printf("each_keylog_comp error!\n");//测试用
    //     return 1;
    // }
    
    if(time_comp(&time_reg)!=0){
        printf("time_comp error!\n");//测试用
        return 1;
    }

    if(address_comp(&address_reg)!=0){
        printf("address_comp error!\n");//测试用
        return 1;
    }
    
    return 0;
}

//如果这是一个死循环,可以不考虑free
u_int8_t log_regex_free(void){
    return 0;
}


//判断一个字符串是否是蓝牙日志
bool is_btlog(char* str){
    if(strstr(str,"bt")==NULL){
        return false;
    }
    return true;
    //return regex_match(&bt_reg , str);//正则表达式版本
}

//判断是否为关键日志(在蓝牙日志的基础上)
bool is_keylog(char* str){
    return regex_match(&keylog_reg , str);
}

//判决是哪一种日志(在关键日志的基础上)
void log_match(char *str){
    singal_log_match(str);
    return;
}