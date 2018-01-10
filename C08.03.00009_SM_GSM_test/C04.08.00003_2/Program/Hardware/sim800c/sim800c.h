#ifndef __SIM800C_H__
#define __SIM800C_H__	 


#include "main.h"
#include "stdint.h"

#define  GSM_BUF_SIZE  1024
#define  GSM_MSG_STOP_FLAG  0x1A

typedef void (*TimeOut_Fun)(void);

/***GSM短信：设置成功***/
extern uint8_t GSM_MSG_SET[];
/***GSM短信：密码设置成功***/
extern uint8_t GSM_MSG_SET_PWD[];
/***GSM短信：目标号码设置成功***/
extern uint8_t GSM_MSG_SET_PHONE[];
/***GSM短信：波长设置成功***/
extern uint8_t GSM_MSG_SET_W[];
/***GSM短信：告警功率设置成功***/
extern uint8_t GSM_MSG_SET_P[];
/***GSM短信：密码错误***/
extern uint8_t GSM_MSG_ERR[];
/***GSM短信：指令错误***/
extern uint8_t GSM_MSG_ERR1[];
/***GSM短信：格式错误***/
extern uint8_t GSM_MSG_ERR2[];
/***GSM短信：设置失败***/
extern uint8_t GSM_MSG_ERR3[];
/***GSM短信：密码格式错误***/
extern uint8_t GSM_MSG_ERR4[];

extern uint8_t GSM_BUF9[];
extern uint8_t GSM_BUSY;

void sim800c_init(uint32_t BPS);
void get_smscenter(void);
void get_IMEI(void);
void sim800c_send(uint8_t *MSG, uint16_t len);
void sim800c_send_to(uint8_t *TARGET, uint8_t *MSG, uint16_t len);
void device_status_get(void);
void device_status_check(void);
void GSM_ALM(void);
void GSM_TCPC_INIT(void);
int GSM_TCPC(void);
int GSM_SMS_W_TCPC(int *ret);
int GSM_SMS_P_TCPC(int *ret);
void GSM_CreatMsgString(char *msg,uint8 statu,float power);
uint8_t GSM_SMS_RCV(void);


#endif


