/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ���ֺ������ͨ�ż������޹�˾

 ******************************************************************************
  �� �� ��   : main.h
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2013��8��20��

******************************************************************************/
#include "includes.h"
#include "timer.h"
#include "my_inet.h"

#ifndef __MAIN_H 
#define __MAIN_H 

#ifdef __cplusplus
extern "C" {
#endif

//#define USE_DEBUG 1	

/******************************************************************************
  �Զ�����±���
*******************************************************************************/
typedef unsigned char  uint8;                   /* defined for unsigned 8-bits integer variable 	�޷���8λ���ͱ���  */
typedef signed   char  int8;                    /* defined for signed 8-bits integer variable		�з���8λ���ͱ���  */
typedef unsigned short uint16;                  /* defined for unsigned 16-bits integer variable 	�޷���16λ���ͱ��� */
typedef signed   short int16;                   /* defined for signed 16-bits integer variable 		�з���16λ���ͱ��� */
typedef unsigned int   uint32;                  /* defined for unsigned 32-bits integer variable 	�޷���32λ���ͱ��� */
typedef signed   int   int32;                   /* defined for signed 32-bits integer variable 		�з���32λ���ͱ��� */
typedef float          fp32;                    /* single precision floating point variable (32bits) �����ȸ�������32λ���ȣ� */
typedef double         fp64;                    /* double precision floating point variable (64bits) ˫���ȸ�������64λ���ȣ� */
							          
/*********************************************************************************************************
  TASK STACK SIZES  �����ջ��С
*********************************************************************************************************/
#define  TASK_STK_SIZE                128

//���Ŷ���
#define  SIM800C_PWRKEY LPC_GPIO1->DIR |= (1<<23)       //sim800c PWRKEY ����Ϊ���
#define  PWRKEY_H       LPC_GPIO1->SET |= (1<<23)   
#define  PWRKEY_L       LPC_GPIO1->CLR |= (1<<23)
//#define  SIM800C_STA LPC_GPIO2->DIR |= (1<<12)       //sim800c STA ����Ϊ���
#define  SIM_STA       ((LPC_GPIO2->PIN >>12)&0x00000001)
#define  REST_PIN_INIT  LPC_IOCON->P2_10 = 0x00000010
#define  REST_PIN       ((LPC_GPIO2->PIN >> 10)&0x00000001)

#define  RX_LED_INIT    LPC_GPIO1->DIR |= (3u<<20)      //R1_LED  ����Ϊ���
#define  R1_OFF         LPC_GPIO1->SET |= (1<<20)
#define  R2_OFF         LPC_GPIO1->SET |= (1<<21)
#define  R1_ON          LPC_GPIO1->CLR |= (1<<20)
#define  R2_ON          LPC_GPIO1->CLR |= (1<<21)

#define  RUN_LED_INIT   LPC_GPIO1->DIR |= (1<<22)       //���е�:����Ϊ��� I/O ��
#define  RUN_LED_H      LPC_GPIO1->SET |= (1<<22)       
#define  RUN_LED_L      LPC_GPIO1->CLR |= (1<<22)

#define  GET_POWER_STAT ((LPC_GPIO0->PIN >> 7) & 0x01)  //��õ�Դ״̬

#define  CHANNEL_NUM       1            //��·���궨��
#define  LOG_ADDR          1500         //��־�洢��ַ
#define  EEPROM_BASE_ADDRESS  EPROM.BPS //flashģ��EPROM����ַ
#define  SIM800C_BAUDRATE 115200
#define RESET_COUNT     5

extern const   uint8   SVersion[];      //ģ������汾��
extern const   uint8   pver[];  
extern struct  EPROM_DATA  EPROM;       //����EPROM���ò����Ľṹ��

extern uint8  GSM_PHONE_temp[];
extern uint8  GSM_CMD_FLAG;
extern uint8  POWER_STAT;
extern uint8  POWER_STAT_OLD;
extern uint8  CHANNEL_STAT_OLD[];
extern uint8_t SET_FLAG ;
extern uint8  u0ReviceBuf[];           //����TaskUart0Revice �õ�����

__packed struct EPROM_DATA {
	
	uint8  BPS;                 //���ڲ�����  1:2400 2:4800 3:9600 4:14400 5:19200 6:38400 7:56000 8:57600 9:115200
	uint8  LOG_NUM;             //��־��
	uint8  address;             //�豸��ַ: 00 ~ 99��
	
	/***GSMģ��������ġ�Ŀ����롢�Զ˺�***/
	uint8_t  GSM_ADDR[19];
	uint8_t  GSM_PHONE[3][29];
	uint8_t  GSM_PASSWORD[7];
	/***Ŀ���������ʵ��Ŀ�����***/
	uint8    PHONE_NUM;
	uint8_t  PHONE[3][13];
	
	uint8_t  SMS_ALT;
	
	char   GUID[16];
	
	int8	ADC_just[CHANNEL_NUM*2][2];
	uint8  fuhao[CHANNEL_NUM*2][2];
//	int8   ADC_just_one[CHANNEL_NUM*2][2];
//	int8   ADC_just_two[CHANNEL_NUM*2][2];
//	uint8  fuhao_one[CHANNEL_NUM*2][2];
//	uint8  fuhao_two[CHANNEL_NUM*2][2];

	uint16 DBM_delay[CHANNEL_NUM*2];
	
  uint8  LINK_num;                          //��·��   
	float  q_power[CHANNEL_NUM*2];            //�澯�����л���
	uint8  wavelength[CHANNEL_NUM*2];         //����        0:1310    1:1550    2:850
	
	unsigned long  serverPORT;               //������IP
	unsigned long  serverIP;                 //�������˿�
	unsigned long  circle_time;               //ˢ��ʱ�� ��λ����
	uint8_t use_TCP;
    
};
extern void  FeedDog(void);
extern void    restore_set(void);             //�ָ��������ú���
extern void    Reset_Handler(void);           //�����λ����

#ifdef __cplusplus
}
#endif

#endif
/*********************************************************************************************************
  END FILE 
*********************************************************************************************************/

