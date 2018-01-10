/******************************************************************************

                  版权所有 (C), 2001-2013, 桂林恒毅金宇通信技术有限公司

 ******************************************************************************
  文 件 名   : main.h
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2013年8月20日

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
  自定义的新变量
*******************************************************************************/
typedef unsigned char  uint8;                   /* defined for unsigned 8-bits integer variable 	无符号8位整型变量  */
typedef signed   char  int8;                    /* defined for signed 8-bits integer variable		有符号8位整型变量  */
typedef unsigned short uint16;                  /* defined for unsigned 16-bits integer variable 	无符号16位整型变量 */
typedef signed   short int16;                   /* defined for signed 16-bits integer variable 		有符号16位整型变量 */
typedef unsigned int   uint32;                  /* defined for unsigned 32-bits integer variable 	无符号32位整型变量 */
typedef signed   int   int32;                   /* defined for signed 32-bits integer variable 		有符号32位整型变量 */
typedef float          fp32;                    /* single precision floating point variable (32bits) 单精度浮点数（32位长度） */
typedef double         fp64;                    /* double precision floating point variable (64bits) 双精度浮点数（64位长度） */
							          
/*********************************************************************************************************
  TASK STACK SIZES  任务堆栈大小
*********************************************************************************************************/
#define  TASK_STK_SIZE                128

//引脚定义
#define  SIM800C_PWRKEY LPC_GPIO1->DIR |= (1<<23)       //sim800c PWRKEY 设置为输出
#define  PWRKEY_H       LPC_GPIO1->SET |= (1<<23)   
#define  PWRKEY_L       LPC_GPIO1->CLR |= (1<<23)
//#define  SIM800C_STA LPC_GPIO2->DIR |= (1<<12)       //sim800c STA 设置为输出
#define  SIM_STA       ((LPC_GPIO2->PIN >>12)&0x00000001)
#define  REST_PIN_INIT  LPC_IOCON->P2_10 = 0x00000010
#define  REST_PIN       ((LPC_GPIO2->PIN >> 10)&0x00000001)

#define  RX_LED_INIT    LPC_GPIO1->DIR |= (3u<<20)      //R1_LED  设置为输出
#define  R1_OFF         LPC_GPIO1->SET |= (1<<20)
#define  R2_OFF         LPC_GPIO1->SET |= (1<<21)
#define  R1_ON          LPC_GPIO1->CLR |= (1<<20)
#define  R2_ON          LPC_GPIO1->CLR |= (1<<21)

#define  RUN_LED_INIT   LPC_GPIO1->DIR |= (1<<22)       //运行灯:设置为输出 I/O 口
#define  RUN_LED_H      LPC_GPIO1->SET |= (1<<22)       
#define  RUN_LED_L      LPC_GPIO1->CLR |= (1<<22)

#define  GET_POWER_STAT ((LPC_GPIO0->PIN >> 7) & 0x01)  //获得电源状态

#define  CHANNEL_NUM       1            //链路数宏定义
#define  LOG_ADDR          1500         //日志存储地址
#define  EEPROM_BASE_ADDRESS  EPROM.BPS //flash模拟EPROM基地址
#define  SIM800C_BAUDRATE 115200
#define RESET_COUNT     5

extern const   uint8   SVersion[];      //模块软件版本号
extern const   uint8   pver[];  
extern struct  EPROM_DATA  EPROM;       //保存EPROM设置参数的结构体

extern uint8  GSM_PHONE_temp[];
extern uint8  GSM_CMD_FLAG;
extern uint8  POWER_STAT;
extern uint8  POWER_STAT_OLD;
extern uint8  CHANNEL_STAT_OLD[];
extern uint8_t SET_FLAG ;
extern uint8  u0ReviceBuf[];           //任务TaskUart0Revice 用的数组

__packed struct EPROM_DATA {
	
	uint8  BPS;                 //串口波特率  1:2400 2:4800 3:9600 4:14400 5:19200 6:38400 7:56000 8:57600 9:115200
	uint8  LOG_NUM;             //日志数
	uint8  address;             //设备地址: 00 ~ 99。
	
	/***GSM模块短信中心、目标号码、对端号***/
	uint8_t  GSM_ADDR[19];
	uint8_t  GSM_PHONE[3][29];
	uint8_t  GSM_PASSWORD[7];
	/***目标号码数、实际目标号码***/
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
	
  uint8  LINK_num;                          //链路数   
	float  q_power[CHANNEL_NUM*2];            //告警功率切换点
	uint8  wavelength[CHANNEL_NUM*2];         //波长        0:1310    1:1550    2:850
	
	unsigned long  serverPORT;               //服务器IP
	unsigned long  serverIP;                 //服务器端口
	unsigned long  circle_time;               //刷新时间 单位：秒
	uint8_t use_TCP;
    
};
extern void  FeedDog(void);
extern void    restore_set(void);             //恢复出厂设置函数
extern void    Reset_Handler(void);           //软件复位函数

#ifdef __cplusplus
}
#endif

#endif
/*********************************************************************************************************
  END FILE 
*********************************************************************************************************/

