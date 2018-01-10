#ifndef __SIM800C_H__
#define __SIM800C_H__	 


#include "main.h"
#include "stdint.h"

#define  GSM_BUF_SIZE  1024
#define  GSM_MSG_STOP_FLAG  0x1A

typedef void (*TimeOut_Fun)(void);

/***GSM���ţ����óɹ�***/
extern uint8_t GSM_MSG_SET[];
/***GSM���ţ��������óɹ�***/
extern uint8_t GSM_MSG_SET_PWD[];
/***GSM���ţ�Ŀ��������óɹ�***/
extern uint8_t GSM_MSG_SET_PHONE[];
/***GSM���ţ��������óɹ�***/
extern uint8_t GSM_MSG_SET_W[];
/***GSM���ţ��澯�������óɹ�***/
extern uint8_t GSM_MSG_SET_P[];
/***GSM���ţ��������***/
extern uint8_t GSM_MSG_ERR[];
/***GSM���ţ�ָ�����***/
extern uint8_t GSM_MSG_ERR1[];
/***GSM���ţ���ʽ����***/
extern uint8_t GSM_MSG_ERR2[];
/***GSM���ţ�����ʧ��***/
extern uint8_t GSM_MSG_ERR3[];
/***GSM���ţ������ʽ����***/
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


