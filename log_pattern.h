#ifndef _LOG_PATTERN_H
#define _LOG_PATTERN_H
#include <sys/types.h>


//虚假的接口,已经改为static
// u_int8_t bt_comp(regex_t *bt_reg);
// u_int8_t regex_match(regex_t *bt_reg , char *str);
// u_int8_t regex_match(regex_t *bt_reg , char *str,regmatch_t &matchptr );
// u_int8_t keylog_comp(regex_t *cmb_reg, regex_t key_reg[]);
// u_int8_t get_conn_info(void);
// u_int8_t get_hf_on_info(char *str);
// u_int8_t get_av_info(char *str);
// u_int8_t get_av_down_info(char *str);
// u_int8_t singal_regex_match(regex_t key_reg[],char *str);

//真 对外接口
u_int8_t log_regex_init(void);
u_int8_t log_regex_free(void);

bool is_btlog(char* str);
bool is_keylog(char* str);
void log_match(char *str);
#endif