/******************************************************************************

                  版权所有 (C), 2001-2013, 桂林恒毅金宇通信技术有限公司

 ******************************************************************************
  文 件 名   : main.c
  版 本 号   : 初稿
  作    者   :
  生成日期   : 2017年8月7日
  最近修改   :
  功能描述   :
******************************************************************************/
#include "config.h"
#include "main.h"
#include "cJSON.h"

#include "queue.h"
#include "Cmd_process.h"

#include "lpc177x_8x_eeprom.h"
#include "ADC.h"
#include "uart0.h"
#include "uart.h"
#include "sim800c.h"
//********************************************************************************
//************************************全局变量定义*******************************
//********************************************************************************
const  uint8  SVersion[]="2.14\0";                //模块软件版本号
const  uint8  pver[]="1\0";  
uint8  Run_flag;                                  //LED 运行灯
uint8  GSM_CMD_FLAG=0;
uint8  POWER_STAT=0;                              //供电状态
uint8  POWER_STAT_OLD=0;                          //供电状态
uint8  CHANNEL_STAT_OLD[CHANNEL_NUM*2];           //Rx状态
uint8_t SET_FLAG = 0;
uint8_t USE_TCP = 0;
uint8 flag = 1;

uint8  GSM_PHONE_temp[29];
uint8  u0ReviceBuf[120];                          //任务TaskUart0Revice 用的数组
struct EPROM_DATA EPROM;                          //保存EPROM设置参数的结构体

OS_EVENT *Uart0ReviceMbox;                        //串口 0 接收邮箱

OS_STK   TaskWDTStk[TASK_STK_SIZE];
OS_STK   TaskCollectStk[TASK_STK_SIZE];
OS_STK   TaskUart0CmdStk[2048];
OS_STK   TaskUart0RcvStk[TASK_STK_SIZE*2];
OS_STK   TaskGSMAlmStk[2048];
OS_STK   TaskGSMRcvStk[2048];
OS_STK   TaskGSMTcpCStk[2048];

//********************************************************************************
//************************************声明任务***********************************
//********************************************************************************
void  TaskWDT(void *pdata);
void  TaskCollect(void *pdata);
void  TaskUart0Cmd(void *pdata);
void  TaskUart0Rcv(void *pdata);
void  TaskGSMAlm(void *pdata);
void  TaskGSMRcv(void *pdata);
void  TaskGSMTcpC(void *pdata);
void  FeedDog(void)
{
	CPU_SR cpu_sr;
	OS_ENTER_CRITICAL();
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
	OS_EXIT_CRITICAL();	
}
/********************************************************************************************************
** 函数名称: restore_set()
** 功能描述: 出厂默认设置
** 输　出:   无
********************************************************************************************************/
void restore_set(void)
{
    uint8 i;
	unsigned long ip;
    EPROM.BPS = 9;            //串口波特率(115200)
    EPROM.address = 1;        //设备地址: 00 ~ 99。
    EPROM.LINK_num = 4;       //链路数
    EPROM.GSM_PASSWORD[0]='6';
	EPROM.SMS_ALT = 1;
	EPROM.GSM_PASSWORD[1]='6';
	EPROM.GSM_PASSWORD[2]='6';
	EPROM.GSM_PASSWORD[3]='6';
	EPROM.GSM_PASSWORD[4]='6';
	EPROM.GSM_PASSWORD[5]='6';
	EPROM.GSM_PASSWORD[6]='\0';
	EPROM.circle_time = 30;
	my_inet_aton("120.78.62.18",&ip);
	EPROM.serverIP = ip;
	EPROM.serverPORT = 6001;
	EPROM.use_TCP = 1;
    for(i=0; i<CHANNEL_NUM*2; i++)
    {
        EPROM.q_power[i] = -30.0;        //告警功率切换点
        EPROM.wavelength[i] = 1;          //波长:1-1550 , 0-1310
    }

    //再给EEPROM初始化,防止EEPROM出问题,无法保存
    LPC1778_EEPROM_Init( );
	OSTimeDly(10);
    LPC1778_EEPROM_Init( );
	Save_To_EPROM((uint8 *)&EPROM.BPS, sizeof(struct EPROM_DATA));
    OSTimeDly(10);
	Save_To_EPROM((uint8 *)&EPROM.BPS, sizeof(struct EPROM_DATA));
    OSTimeDly(10);
	OSTimeDly(1000);
}

/********************************************************************************************************
** 函数名称: systemInt
** 功能描述: 初始化
** 输　出:
********************************************************************************************************/
void READ_EPROM_Init(void)
{
    uint8 i;
	unsigned long ip = 0;
    uint8_t temp[17];
    LPC1778_EEPROM_Init();     //I2C 初始化

    EEPROM_Read_Str( 0x00, (uint8 *)&EEPROM_BASE_ADDRESS, sizeof(struct EPROM_DATA) );
    delay_nms(20);
    //再读一次防止出现误码
    EEPROM_Read_Str( 0x00, (uint8 *)&EEPROM_BASE_ADDRESS, sizeof(struct EPROM_DATA) );
	
	EPROM.BPS = 9; 
    EPROM.address = 1;
    if(EPROM.circle_time == 0)
	{
		EPROM.circle_time = 30;
	}
	ip = EPROM.serverIP;
    my_inet_ntoa((char *)temp,ip);				
	//sprintf((char *)GSM_BUF9,"AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r\n",(char *)temp,(int)EPROM.serverPORT);
	for(i=0; i<CHANNEL_NUM*2; i++)
		EPROM.DBM_delay[i]=463;
}


/*******************************************************************************************************
** 函数名称: main
** 功能描述: 主函数
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
int main (void)
{
	RUN_LED_INIT;
	RX_LED_INIT;
//	ADC_INIT;
	SystemInit();
  
	IntDisAll();  // Note:由于使用UCOS, 在OS运行之前运行,注意别使能任何中断.
	OSInit();
	OS_CPU_SysTickInit(SystemCoreClock/OS_TICKS_PER_SEC);

	OSTaskCreate(TaskWDT,       (void *)0, &TaskWDTStk[TASK_STK_SIZE - 1],         1);
	OSTaskCreate(TaskCollect,   (void *)0, &TaskCollectStk[TASK_STK_SIZE- 1],      2);
	OSTaskCreate(TaskUart0Cmd,  (void *)0, &TaskUart0CmdStk[2048 - 1],  3);
	OSTaskCreate(TaskUart0Rcv,  (void *)0, &TaskUart0RcvStk[TASK_STK_SIZE*2 - 1],  4);
	OSTaskCreate(TaskGSMAlm,    (void *)0, &TaskGSMAlmStk[2048 - 1],      5);
	OSTaskCreate(TaskGSMRcv,    (void *)0, &TaskGSMRcvStk[2048 - 1],      6);
	OSTaskCreate(TaskGSMTcpC,   (void *)0, &TaskGSMTcpCStk[2048 - 1],     7);

	//建立串口0的接收邮箱
	Uart0ReviceMbox = OSMboxCreate(NULL);
	if(Uart0ReviceMbox == NULL)
	{
		while (1);
	}
	
	OSStart ();
	return 0;
}

/********************************************************************************************************
** 函数名称: TaskWDT
** 功能描述: 看门狗复位
** 输　入: 无
** 输　出: 无
** 说  明: 看门狗使用内部RC时钟(4MHz),经过4次分频。T=0x1000000*1us=1s (看门狗复位定时时间)
********************************************************************************************************/
void TaskWDT(void *pdata)
{
	OS_CPU_SR  cpu_sr;

	READ_EPROM_Init();      //读EPROM初始化参数
	UART0Init();            //串口初始化(必须先读EPROM 中的串口配置叁数后才能初始化 )

	/*******************开启看门狗********************/
	/**LPC1778的看门狗使用内部RC时钟(500KHz),经过4次分频(500K/4=125K ,即十六进制为0X1E848)**/
	LPC_WDT->TC  = 0X1E848;  ; //设置WDT定时值为1秒.
	LPC_WDT->MOD = 0x03;       //设置WDT工作模式,启动WDT
	REST_PIN_INIT;
	while(1)
	{
		
    OS_ENTER_CRITICAL();
		LPC_WDT->FEED = 0xAA;
		LPC_WDT->FEED = 0x55;
		OS_EXIT_CRITICAL();																
    

		if(Run_flag == 1)
		{
			RUN_LED_H;
			Run_flag=0;
		}
		else if(Run_flag == 0)
		{
			RUN_LED_L;
			Run_flag=1;
		}
		if(!REST_PIN)
		{
				
				PWRKEY_L;
				delay_nms(2000);
				PWRKEY_H;
				while(SIM_STA)
				{
				 delay_nms(1);
				}
				delay_nms(1000);
				PWRKEY_L;
				delay_nms(2000);
				PWRKEY_H;		
				Reset_Handler(); 
		}
		OSTimeDly(500);
	}
}

/********************************************************************************************************
** 函数名称: TaskCollect
** 功能描述: R1、R2通道功率采集
** 输　入:   pdata 任务附加参数(指针)
** 输　出: 无
** 运算时间 :
********************************************************************************************************/
void TaskCollect(void *pdata)
{
	uint8_t i;

	for(i=0; i<2; i++)
	{
		hardware_way[i] = 1;        //初始化模拟开关位置
		CHANNEL(i,4);               //设定到第4通道
	}

	OSTimeDly(20);
	ADC_int(400);//ADC初始化
	
	while(1)
	{
		optics_collect(1, hardware_way[1]);
		OSTimeDly(200);
        optics_collect(0, hardware_way[0]);
		OSTimeDly(200);
		
		
	}
}

/********************************************************************************************************
** 函数名称: TaskUart0Cmd
** 功能描述: 命令解析
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskUart0Cmd(void* pdata)
{
	uint8  err;
	uint16 len;

	OSTimeDly(500);            //等待延时
	while(1)
	{
		OSMboxPend(Uart0ReviceMbox, 0, &err);         // 等待接收邮箱数据
		GSM_CMD_FLAG = 0;
		if( (len = Cmd_process((char*)&u0ReviceBuf)) > 0 )
			UART0Put_str(u0ReviceBuf, len);
	}
}

/********************************************************************************************************
** 函数名称: TaskUart0Rcv
** 功能描述: μCOS-II的任务。从UART0接收数据，当接收完一帧数据后通过消
**           息邮箱传送到TaskStart任务。
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskUart0Rcv(void* pdata)
{
	uint8 *cp;
	uint8 i,temp;

	while(1)
	{
		cp = u0ReviceBuf;
		while((*cp = UART0Getch()) != '<') ;  // 接收数据头
		cp++;   								              //往下移一个字节
		for (i = 0; i < 50; i++)
		{
			temp = UART0Getch();
			*cp++ = temp;
			if (temp =='>')
			{
				while(i < 48)
				{
					*cp++ = '\0';                     //空余的后面补0
					i++;
				}
				break;
			}
		}
		OSMboxAccept(Uart0ReviceMbox);       //清空 邮箱Uart0ReviceMbox
		OSMboxPost(Uart0ReviceMbox, (void *)u0ReviceBuf);
	}
}

/********************************************************************************************************
** 函数名称: TaskGSMAlt
** 功能描述: GSM告警
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskGSMAlm(void* pdata)
{	
	CPU_SR cpu_sr;
	uint8_t xxtemp[16];
	sprintf((char *)xxtemp, "<BP%02u_RESET_OK>\r\n", EPROM.address);
	UART0Write_Str((uint8 *)xxtemp); 
	sim800c_init(SIM800C_BAUDRATE);
	get_smscenter();
	OSTimeDly(10);
	get_IMEI();
	OSTimeDly(10);
	GSM_TCPC_INIT();
	OSTimeDly(5000);
	OS_ENTER_CRITICAL();
    flag = 0;	
	OS_EXIT_CRITICAL();
    while(1)
	{
		device_status_check();
		OSTimeDly(2);
		GSM_ALM();
		OSTimeDly(1000);
	}
}

/********************************************************************************************************
** 函数名称: TaskGSMRcv
** 功能描述: GSM短信接收
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskGSMRcv(void* pdata)
{
    while(flag)
    {
		OSTimeDly(1500);
	}
     while(1)
	{
		GSM_SMS_RCV();
		OSTimeDly(5000);
	}
}

/********************************************************************************************************
** 函数名称: TaskGSMTcpC
** 功能描述: GSM/TCP客户端
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskGSMTcpC(void* pdata)
{
	uint8 timecut;
	while(flag)
	{
		OSTimeDly(2000);
	}	
    while(1)
	{
		OSTimeDly(1000);
		timecut++;
		if(timecut >=EPROM.circle_time)
		{
			GSM_TCPC();
		    timecut = 0;
		}
	}
}
/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
