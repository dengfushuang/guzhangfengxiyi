/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ���ֺ������ͨ�ż������޹�˾

 ******************************************************************************
  �� �� ��   : main.c
  �� �� ��   : ����
  ��    ��   :
  ��������   : 2017��8��7��
  ����޸�   :
  ��������   :
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
//************************************ȫ�ֱ�������*******************************
//********************************************************************************
const  uint8  SVersion[]="2.14\0";                //ģ������汾��
const  uint8  pver[]="1\0";  
uint8  Run_flag;                                  //LED ���е�
uint8  GSM_CMD_FLAG=0;
uint8  POWER_STAT=0;                              //����״̬
uint8  POWER_STAT_OLD=0;                          //����״̬
uint8  CHANNEL_STAT_OLD[CHANNEL_NUM*2];           //Rx״̬
uint8_t SET_FLAG = 0;
uint8_t USE_TCP = 0;
uint8 flag = 1;

uint8  GSM_PHONE_temp[29];
uint8  u0ReviceBuf[120];                          //����TaskUart0Revice �õ�����
struct EPROM_DATA EPROM;                          //����EPROM���ò����Ľṹ��

OS_EVENT *Uart0ReviceMbox;                        //���� 0 ��������

OS_STK   TaskWDTStk[TASK_STK_SIZE];
OS_STK   TaskCollectStk[TASK_STK_SIZE];
OS_STK   TaskUart0CmdStk[2048];
OS_STK   TaskUart0RcvStk[TASK_STK_SIZE*2];
OS_STK   TaskGSMAlmStk[2048];
OS_STK   TaskGSMRcvStk[2048];
OS_STK   TaskGSMTcpCStk[2048];

//********************************************************************************
//************************************��������***********************************
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
** ��������: restore_set()
** ��������: ����Ĭ������
** �䡡��:   ��
********************************************************************************************************/
void restore_set(void)
{
    uint8 i;
	unsigned long ip;
    EPROM.BPS = 9;            //���ڲ�����(115200)
    EPROM.address = 1;        //�豸��ַ: 00 ~ 99��
    EPROM.LINK_num = 4;       //��·��
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
        EPROM.q_power[i] = -30.0;        //�澯�����л���
        EPROM.wavelength[i] = 1;          //����:1-1550 , 0-1310
    }

    //�ٸ�EEPROM��ʼ��,��ֹEEPROM������,�޷�����
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
** ��������: systemInt
** ��������: ��ʼ��
** �䡡��:
********************************************************************************************************/
void READ_EPROM_Init(void)
{
    uint8 i;
	unsigned long ip = 0;
    uint8_t temp[17];
    LPC1778_EEPROM_Init();     //I2C ��ʼ��

    EEPROM_Read_Str( 0x00, (uint8 *)&EEPROM_BASE_ADDRESS, sizeof(struct EPROM_DATA) );
    delay_nms(20);
    //�ٶ�һ�η�ֹ��������
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
** ��������: main
** ��������: ������
** �䡡��: ��
** �䡡��: ��
********************************************************************************************************/
int main (void)
{
	RUN_LED_INIT;
	RX_LED_INIT;
//	ADC_INIT;
	SystemInit();
  
	IntDisAll();  // Note:����ʹ��UCOS, ��OS����֮ǰ����,ע���ʹ���κ��ж�.
	OSInit();
	OS_CPU_SysTickInit(SystemCoreClock/OS_TICKS_PER_SEC);

	OSTaskCreate(TaskWDT,       (void *)0, &TaskWDTStk[TASK_STK_SIZE - 1],         1);
	OSTaskCreate(TaskCollect,   (void *)0, &TaskCollectStk[TASK_STK_SIZE- 1],      2);
	OSTaskCreate(TaskUart0Cmd,  (void *)0, &TaskUart0CmdStk[2048 - 1],  3);
	OSTaskCreate(TaskUart0Rcv,  (void *)0, &TaskUart0RcvStk[TASK_STK_SIZE*2 - 1],  4);
	OSTaskCreate(TaskGSMAlm,    (void *)0, &TaskGSMAlmStk[2048 - 1],      5);
	OSTaskCreate(TaskGSMRcv,    (void *)0, &TaskGSMRcvStk[2048 - 1],      6);
	OSTaskCreate(TaskGSMTcpC,   (void *)0, &TaskGSMTcpCStk[2048 - 1],     7);

	//��������0�Ľ�������
	Uart0ReviceMbox = OSMboxCreate(NULL);
	if(Uart0ReviceMbox == NULL)
	{
		while (1);
	}
	
	OSStart ();
	return 0;
}

/********************************************************************************************************
** ��������: TaskWDT
** ��������: ���Ź���λ
** �䡡��: ��
** �䡡��: ��
** ˵  ��: ���Ź�ʹ���ڲ�RCʱ��(4MHz),����4�η�Ƶ��T=0x1000000*1us=1s (���Ź���λ��ʱʱ��)
********************************************************************************************************/
void TaskWDT(void *pdata)
{
	OS_CPU_SR  cpu_sr;

	READ_EPROM_Init();      //��EPROM��ʼ������
	UART0Init();            //���ڳ�ʼ��(�����ȶ�EPROM �еĴ���������������ܳ�ʼ�� )

	/*******************�������Ź�********************/
	/**LPC1778�Ŀ��Ź�ʹ���ڲ�RCʱ��(500KHz),����4�η�Ƶ(500K/4=125K ,��ʮ������Ϊ0X1E848)**/
	LPC_WDT->TC  = 0X1E848;  ; //����WDT��ʱֵΪ1��.
	LPC_WDT->MOD = 0x03;       //����WDT����ģʽ,����WDT
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
** ��������: TaskCollect
** ��������: R1��R2ͨ�����ʲɼ�
** �䡡��:   pdata ���񸽼Ӳ���(ָ��)
** �䡡��: ��
** ����ʱ�� :
********************************************************************************************************/
void TaskCollect(void *pdata)
{
	uint8_t i;

	for(i=0; i<2; i++)
	{
		hardware_way[i] = 4;        //��ʼ��ģ�⿪��λ��
		CHANNEL(i,hardware_way[i]);               //�趨����4ͨ��
	}

	OSTimeDly(20);
	ADC_int(400);//ADC��ʼ��
	
	while(1)
	{
		optics_collect(1, hardware_way[1]);
		OSTimeDly(200);
        optics_collect(0, hardware_way[0]);
		OSTimeDly(200);
		
		
	}
}

/********************************************************************************************************
** ��������: TaskUart0Cmd
** ��������: �������
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
********************************************************************************************************/
void TaskUart0Cmd(void* pdata)
{
	uint8  err;
	uint16 len;

	OSTimeDly(500);            //�ȴ���ʱ
	while(1)
	{
		OSMboxPend(Uart0ReviceMbox, 0, &err);         // �ȴ�������������
		GSM_CMD_FLAG = 0;
		if( (len = Cmd_process((char*)&u0ReviceBuf)) > 0 )
			UART0Put_str(u0ReviceBuf, len);
	}
}

/********************************************************************************************************
** ��������: TaskUart0Rcv
** ��������: ��COS-II�����񡣴�UART0�������ݣ���������һ֡���ݺ�ͨ����
**           Ϣ���䴫�͵�TaskStart����
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
********************************************************************************************************/
void TaskUart0Rcv(void* pdata)
{
	uint8 *cp;
	uint8 i,temp;

	while(1)
	{
		cp = u0ReviceBuf;
		while((*cp = UART0Getch()) != '<') ;  // ��������ͷ
		cp++;   								              //������һ���ֽ�
		for (i = 0; i < 50; i++)
		{
			temp = UART0Getch();
			*cp++ = temp;
			if (temp =='>')
			{
				while(i < 48)
				{
					*cp++ = '\0';                     //����ĺ��油0
					i++;
				}
				break;
			}
		}
		OSMboxAccept(Uart0ReviceMbox);       //��� ����Uart0ReviceMbox
		OSMboxPost(Uart0ReviceMbox, (void *)u0ReviceBuf);
	}
}

/********************************************************************************************************
** ��������: TaskGSMAlt
** ��������: GSM�澯
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
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
** ��������: TaskGSMRcv
** ��������: GSM���Ž���
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
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
** ��������: TaskGSMTcpC
** ��������: GSM/TCP�ͻ���
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
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
