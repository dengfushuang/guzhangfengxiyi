#include "sim800c.h"
#include "uart.h"
#include "uart0.h"
#include "ADC.h"
#include "main.h"
#include "Cmd_process.h"
#include "log.h"
#include "lpc177x_8x_eeprom.h"
#include "cJSON.h"
#include "unicode.h"
#include "delay.h"
#include "stdio.h"

char * CreateINFOCommand(void);
char * CreateBACommand(void);
char * GenerateINFOResult(int code);
char * GenerateQBWResult(int code);

uint8  sim800c_RCV_BUF[60];
uint8  GSM_RCV_BUF[GSM_BUF_SIZE];

unsigned int reboot = 0;

/***GSM SMS***/
uint8_t  GSM_BUF0[] = "AT\r\n";
uint8_t  GSM_BUF1[] = "AT+CMGF=0\r\n";
uint8_t  GSM_BUF3[] = "AT+CMGL=\"REC UNREAD\"\r\n";
uint8_t  GSM_BUF4[] = "AT+CNMI=\"GSM\"\r\n";
uint8_t  GSM_BUF5[] = "AT+CMGF=1\r\n";
/***GSM TCP***/
uint8_t  GSM_BUF6[] = "AT+CSTT=\"CMNET\"\r\n";
uint8_t  GSM_BUF7[] = "AT+CIICR\r\n";
uint8_t  GSM_BUF8[] = "AT+CIFSR\r\n";
uint8_t  GSM_BUF9[] = "AT+CIPSTART=\"TCP\",\"120.78.62.18\",\"6001\"\r\n";

uint8_t  GSM_CIPSRIP[] = "AT+CIPSRIP=1\r\n";
uint8_t  GSM_CIPSEND[] = "AT+CIPSEND\r\n";
uint8_t  GSM_CIPCLOSE[] = "AT+CIPCLOSE\r\n";
uint8_t  GSM_CIPSHUT[] = "AT+CIPSHUT\r\n";


/***GSM短信：设置成功***/
uint8_t GSM_MSG_SET[19] = "088BBE7F6E6210529F\0";
/***GSM短信：密码设置成功***/
uint8_t GSM_MSG_SET_PWD[27] = "0C5BC678018BBE7F6E6210529F\0";
/***GSM短信：告警号码设置成功***/
uint8_t GSM_MSG_SET_PHONE[35] = "10544A8B6653F778018BBE7F6E6210529F\0";
/***GSM短信：工作波长设置成功***/
uint8_t GSM_MSG_SET_W[35] = "105DE54F5C6CE2957F8BBE7F6E6210529F\0"; 
//uint8_t GSM_MSG_SET_W[27] = "0C6CE2957F8BBE7F6E6210529F\0";
/***GSM短信：告警功率设置成功***/
uint8_t GSM_MSG_SET_P[35] = "10544A8B66529F73878BBE7F6E6210529F\0";

/***GSM短信：密码错误***/
uint8_t GSM_MSG_ERR[19] = "085BC6780195198BEF\0";
/***GSM短信：指令错误***/
uint8_t GSM_MSG_ERR1[19] = "0863074EE495198BEF\0";
///***GSM短信：格式错误***/
//uint8_t GSM_MSG_ERR2[19] = "08683C5F0F95198BEF\0";
/***GSM短信：设置失败***/
uint8_t GSM_MSG_ERR3[19] = "088BBE7F6E59318D25\0";
/***GSM短信：密码格式错误***/
uint8_t GSM_MSG_ERR4[27] = "0C5BC67801683C5F0F95198BEF\0";

/***串口返回信息***/
uint8_t U0_ERR[] = "SMS TIMEOUT!\r\n\0";

/***正在处理告警信息***/
uint8_t ALM_FLAG = 0, GSM_BUSY;
uint8_t PWR_ALM_FLAG, CH_ALM_FLAG;
/***Rx工作波长***/
uint8_t MSG_WAVELEN[CHANNEL_NUM*2][57];
/***Rx状态***/
uint8_t MSG_CHANNEL[CHANNEL_NUM*2][81];
/***Rx告警功率***/
uint8_t MSG_q_power[CHANNEL_NUM*2][69];
uint8_t MSG_CHANNEL_STAT[CHANNEL_NUM*2][85];
uint8_t MSG_PHONE[3][77];

uint8_t GSM_MSG_BUF[300];
static int err_count = 0;
static int err_count1 = 0;

void timeout_err(void)
{
	;
}
/*********************************************************************************************************
** 函数名称: end_of_string
** 功能描述: 当计算值超过10次未接收到数据，认为数据已经接收完毕
** 输　入: 变量地址 ncount
** 输　出: 无
********************************************************************************************************/
int end_of_string(int *ncount)
{
	*ncount +=1;
	if(*ncount > 3000)
	{
		return -1;
	}
	return 1;
}
/*********************************************************************************************************
** 函数名称: deal_string
** 功能描述: 字符串处理
** 输　入: buf-数据缓冲区,str-比较字符串,buf_len-缓冲区长度，timeout-超时时间,timeout_fun-超时回调函数；
** 输　出: 无
********************************************************************************************************/
int deal_string(char *str,uint16_t str_len,uint16_t timeout)
{
	uint8_t data_temp,slen = 0;
	char *cp1,*cp2;
	int count = 0,t = 0,i=0;
	cp1 = str;
	cp2 = str;
	slen = str_len;
	while(i  < slen)
	{
		data_temp= UART1GetFun(&count);
		if(data_temp >= 32 && data_temp <=127)
		{
			count = 0;
			if( *cp1 == data_temp)
			{
				
				cp1++;
				i++;
				if( *cp1 == '\0')
				{
					break;
				}
				if(cp1 == NULL )
				{
					count = 0;
					return -1;
				}
				
			}
			else
			{
				cp1 = cp2;
				i = 0;
				if(*cp1 == data_temp)
				{
					
					cp1 ++;
					i++;
					if(*cp1 == '\0')
				    {
					    break;
				    }
					if(cp1 == NULL )
				    {
					    count = 0;
					    return -1;
				    }
					
				}
			}  
		}
		else
		{
			t++;
			if(t >= 5000)
			{
				t = 0;
				FeedDog();
			}
		}
		if(i >= slen)
		{
			break;
		}
		if(count > timeout )
		{
			count = 0;
			return -1;
		}
	}
	if(i == slen )
	{
		return 1;
	}else
	{
		return 0;
	}
}

/*********************************************************************************************************
** 函数名称: get_string
** 功能描述: 字符串接收
** 输　入: buf-数据缓冲区,str-比较字符串,buf_len-缓冲区长度，timeout-超时时间,timeout_fun-超时回调函数；
** 输　出: 无
********************************************************************************************************/
int get_string(uint8_t buf[],uint16_t buf_len,int timeout)
{
	uint8_t data_temp;
	int count = 0;
	int ch_count;
	if(buf == NULL)
	{
		return -1;
	}
	for(count = 0 ;count < (buf_len-1);count ++)
	{
		buf[count] = 0;
	}
	count = 0;
	ch_count = 0;
	while(count < timeout)
	{   
		data_temp = UART1GetFun(&count);
		if(count < timeout)
		{
			if(data_temp >=32 && data_temp <= 127)
			{
				count = 0;
				if(ch_count >= (buf_len-1))
				{
					return -1;
				}
				buf[ch_count] = data_temp;
				ch_count++;
			}
		}
	}
	return 1;
}

int check_ststus(uint8_t *sendstr,char *str,uint16_t str_len,uint16_t timeout,uint16_t resend)
{
	int cnt = 0;
	int temp ;
	int err = 1;
	UART1Write_Str(sendstr);
	temp = deal_string(str,str_len,timeout);
	while(temp < 0)
	{
		cnt++;
		if(cnt > resend)
		{
			err =  -1;
			break;
		}
		UART1Write_Str(sendstr);
		temp = deal_string(str,str_len,timeout);
	}
	return err;
}


/*********************************************************************************************************
** 函数名称: sim800c_init
** 功能描述: GSM模块初始化
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void sim800c_init(uint32_t BPS)
{
	int temp,cnt=0;
    UART1_Init(BPS);
	
	/***SIM800C电源控制脚初始化***/
    SIM800C_PWRKEY;
	PWRKEY_H;
	
	OSTimeDly(5000);  //确保GSM模块搜索到网络后再进入系统
	
	
	UART1Write_Str(GSM_BUF0);
	OSTimeDly(100);
RECIEVE_READY:
	temp = deal_string("Call",4,5000);
	temp = deal_string("SMS",3,5000);
	if(temp < 0)
	{
		cnt ++;
		if(cnt < 4)
		{
			goto RECIEVE_READY;
		}
	}
	OSTimeDly(100);
	//UART1Write_Str(GSM_BUF1);
	if(check_ststus(GSM_BUF1,"OK",2,2000,4) < 0)
	{
		;
	}
	//UART1Write_Str((uint8_t *)"AT+CMGDA=6\r\n");
	if(check_ststus((uint8_t *)"AT+CMGDA=6\r\n","OK",2,2000,4) < 0)
	{
		;
	}
	//UART1Write_Str(GSM_BUF5);
	if(check_ststus(GSM_BUF5,"OK",2,2000,10) < 0)
	{
		;
	}
	//UART1Write_Str((uint8_t *)"AT+CMGDA=\"DEL ALL\"\r\n");
	if(check_ststus((uint8_t *)"AT+CMGDA=\"DEL ALL\"\r\n","OK",2,2000,4) < 0)
	{
		;
	}
	OSTimeDly(5000); 
	
	EPROM.GSM_ADDR[0] = '0';
	EPROM.GSM_ADDR[1] = '8';
	EPROM.GSM_ADDR[2] = '9';
	EPROM.GSM_ADDR[3] = '1';
	EPROM.GSM_ADDR[4] = '6';
	EPROM.GSM_ADDR[5] = '8';
	
	GSM_PHONE_temp[0] = '1';
	GSM_PHONE_temp[1] = '1';
	GSM_PHONE_temp[2] = '0';
	GSM_PHONE_temp[3] = '0';
	GSM_PHONE_temp[4] = '0';
	GSM_PHONE_temp[5] = 'D';
	GSM_PHONE_temp[6] = '9';
	GSM_PHONE_temp[7] = '1';
	GSM_PHONE_temp[8] = '6';
	GSM_PHONE_temp[9] = '8';
	
	GSM_PHONE_temp[22] = '0';
	GSM_PHONE_temp[23] = '0';
	GSM_PHONE_temp[24] = '0';
	GSM_PHONE_temp[25] = '8';
	GSM_PHONE_temp[26] = '0';
	GSM_PHONE_temp[27] = '0';
	GSM_PHONE_temp[28] = '\0';
}

/*********************************************************************************************************
** 函数名称: GSM_TCPC_INIT
** 功能描述: GSM TCP客户端初始化
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void GSM_TCPC_INIT(void)
{
	
	OSTimeDly(3000);
	UART1Write_Str(GSM_CIPSRIP);
	OSTimeDly(2000);
}

/*********************************************************************************************************
** 函数名称: get_sms_center
** 功能描述: 换取短信中心
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void get_smscenter(void)
{
	uint8_t data[60] = {0}, addr_temp[13];
	uint16_t i;
	int temp;
	char *cp;
	unsigned int t3= 0;
	
check:
	
	i=0;
	UART1Write_Str((uint8_t *)"AT+CSCA?\r\n");
	temp  = deal_string("+CSCA:",6,3000); 
	if(temp <= 0)
	{
		t3++;
		if(t3 >= RESET_COUNT)
		{
			t3 = 0;
			 PWRKEY_L;
			 OSTimeDly(2000);
			 PWRKEY_H;
			 while(SIM_STA)
			 {
				 OSTimeDly(1);
			 }
			 OSTimeDly(1000);
			 PWRKEY_L;
			 OSTimeDly(2000);
			 PWRKEY_H;
		}
		OSTimeDly(1000);
		goto check;
	}
	i = 0;
	/***目标信息***/
	get_string(data,50,1000);
	if( (cp = strstr((char*)&data[0], "+86")) != NULL)
	{
		cp+=3;
		for(i=0;i<11;i++)
		{
			addr_temp[i] = *cp++;
		}
		addr_temp[11] = 'F';
		addr_temp[12] = '\0';
		UART0Write_Str((uint8_t *)"SMS CENTER: ");
		UART0Write_Str(addr_temp);
		UART0Putch('\n');
		for(i=0;i<6;i++)
		{
			EPROM.GSM_ADDR[(2*i)+7] = addr_temp[(2*i)];
		}
		for(i=0;i<6;i++)
		{
			EPROM.GSM_ADDR[(2*i)+6] = addr_temp[(2*i)+1];
		}
		EPROM.GSM_ADDR[18] = '\0';
		Save_To_EPROM((uint8 *)&EPROM.GSM_ADDR, 19);
	}
}

/*********************************************************************************************************
** 函数名称: get_IMEI
** 功能描述: 换取全球唯一标识符
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void get_IMEI(void)
{
	uint8_t data[60] = {0};
	uint16_t i;
	int temp;
	uint8_t *cp;
	unsigned int t3 = 0;
	
check:
	
	i=0;
	UART1Write_Str((uint8_t *)"AT+GSN\r\n");
	temp  = deal_string("+GSN",4,3000); 
	if(temp <= 0)
	{
		t3++;
		if(t3 >= RESET_COUNT)
		{
			t3 = 0;
			 PWRKEY_L;
			 OSTimeDly(2000);
			 PWRKEY_H;
			 while(SIM_STA)
			 {
				 OSTimeDly(1);
			 }
			 OSTimeDly(1000);
			 PWRKEY_L;
			 OSTimeDly(2000);
			 PWRKEY_H;
		}
		OSTimeDly(1000);
		goto check;
	}
	i = 0;
	/***目标信息***/
	get_string(data,50,1000);
	cp = data;
	if( *cp >= '0' && *cp <= '9')
	{
		
		for(i=0;i<15;i++)
		{
			EPROM.GUID[i] = *cp;
			cp++;
		}
		EPROM.GUID[15] = '\0';
		UART0Write_Str((uint8_t *)"GUID: ");
		UART0Write_Str((uint8_t *)EPROM.GUID);
		UART0Putch('\n');
		Save_To_EPROM((uint8 *)&EPROM.GUID[0],16);
	}
}

/*********************************************************************************************************
** 函数名称: sim800c_send
** 功能描述: GSM模块发送
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void sim800c_send(uint8_t *MSG, uint16_t len)
{
	uint8_t  i, data[6], GSM_BUF2[13];
	uint16_t j, MSG_SUM;
	
	MSG_SUM = (30 + len)/2;
	if(MSG_SUM>99)
		sprintf((char *)GSM_BUF2, "AT+CMGS=%3d\r", MSG_SUM);
	else
	    sprintf((char *)GSM_BUF2, "AT+CMGS=%2d\r", MSG_SUM);

	for(i=0;i<EPROM.PHONE_NUM;i++)
	{
		UART1Putch(GSM_MSG_STOP_FLAG);
		delay_nms(100);
        UART1Write_Str(GSM_BUF1);
	    delay_nms(10);
		UART1Write_Str((uint8_t *)GSM_BUF2);
		delay_nms(10);
		UART1Write_Str(EPROM.GSM_ADDR);
		UART1Write_Str(&EPROM.GSM_PHONE[i][0]);
		UART1Write_Str(MSG);
		delay_nms(10);
		UART1Putch(GSM_MSG_STOP_FLAG);
		delay_nms(10);
	wait:	
		j=0;
		while((data[0] = UART1Get()) != '+')
		{
			j++;
			if(j==4500)
			{
			  UART1Putch(GSM_MSG_STOP_FLAG);
			}
			else if(j==9000)
			{
			  UART0Write_Str(U0_ERR);
				return;
			}
		}
		for(j=1;j<6;j++)
		{
			data[j] = UART1Get();
		}
		if(strstr((char*)&data[0], "+CMGS:") == NULL)
		  goto wait;
		else
			continue;
	}
}


/*********************************************************************************************************
** 函数名称: sim800c_send
** 功能描述: GSM模块发送
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void sim800c_send_to(uint8_t *TARGET, uint8_t *MSG, uint16_t len)
{
	uint8_t  data, GSM_BUF2[13];
	uint16_t j, MSG_SUM;
	
	MSG_SUM = (30 + len)/2;
	if(MSG_SUM>99)
		sprintf((char *)GSM_BUF2, "AT+CMGS=%3d\r", MSG_SUM);
	else
	    sprintf((char *)GSM_BUF2, "AT+CMGS=%2d\r", MSG_SUM);

	UART1Putch(GSM_MSG_STOP_FLAG);
	delay_nms(100);
	UART1Write_Str(GSM_BUF1);
	delay_nms(10);
	UART1Write_Str((uint8_t *)GSM_BUF2);
	delay_nms(10);
	UART1Write_Str(EPROM.GSM_ADDR);
	UART1Write_Str(TARGET);
	UART1Write_Str(MSG);
	delay_nms(10);
	UART1Putch(GSM_MSG_STOP_FLAG);
	delay_nms(10);

	j=0;
	while((data = UART1Get()) != '+')
	{
		j++;
		if(j==4500)
		{
			UART1Putch(GSM_MSG_STOP_FLAG);
		}
		else if(j>=9000)
		{
			UART0Write_Str(U0_ERR);
			return;
		}
	}
}


/*********************************************************************************************************
** 函数名称: sim800c_read
** 功能描述: GSM模块接收
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
uint8_t GSM_SMS_RCV(void)
{
	CPU_SR cpu_sr;
	uint8_t  *cp, phone_temp[16];	
	uint8_t  data[300], cmd_temp[50];
	uint16_t i;
	uint8_t *cp_s,*cp_e;
	
	/***接收信息头***/
    while(GSM_BUSY == 1)
	{
		OSTimeDly(1);
	}
    if(GSM_BUSY == 0)
	{
	    GSM_BUSY = 1;	
	/***读取未读短信***/
		delay_nms(10);
		UART1Write_Str(GSM_BUF5);
		delay_nms(10);
		UART1Write_Str(GSM_BUF4);
		delay_nms(10);
		UART1Write_Str(GSM_BUF3);
		/***截取短信中指令内容***/
        get_string(data,300,5000);
		/***目标信息***/
#ifdef USE_DEBUG
		UART0Write_Str(data);
#endif
		if(strstr((char*)&data[0], "+CMGL:") == NULL)
		{
			GSM_BUSY = 0;
			return 0;				
		}
		/***目标信息***/
		
		if((cp = (uint8_t *)strstr((char*)&data[0], "+86")) == NULL)
		{
			GSM_BUSY = 0;
			return 0;
		}
		else
		{
			for(i=0;i<14;i++)
		    {
			    phone_temp[i] = *(cp++);
		    }
			phone_temp[14] = 'F';
		    phone_temp[15] = '\0';
#ifdef USE_DEBUG
			UART0Write_Str(phone_temp);
#endif
			for(i=0;i<6;i++)
			{
				GSM_PHONE_temp[(2*i)+11] = phone_temp[(2*i)+3];
			}
			for(i=0;i<6;i++)
			{
				GSM_PHONE_temp[(2*i)+10] = phone_temp[(2*i)+4];
			}
		}
		i = 0;
		cp = sim800c_RCV_BUF;

		cp_s = (uint8_t *)strstr((char *)data,"<");
		cp_e = (uint8_t *)strstr((char *)data,">");
		strncpy((char *)sim800c_RCV_BUF,(char *)cp_s,(strlen((char *)cp_s)-strlen((char *)cp_e)+1));
		/***检验对端号***/
		for(i=0;i<6;i++)
		{
			if(EPROM.GSM_PASSWORD[i] != sim800c_RCV_BUF[i+1])
			{
				
         #ifdef USE_DEBUG
				UART0Write_Str(GSM_PHONE_temp);
		 #endif
				
				sim800c_send_to(GSM_PHONE_temp, GSM_MSG_ERR, 16);
				GSM_BUSY = 0;
				return 3;
			}
		}
		/***状态查询***/
		if(strstr((char*)&sim800c_RCV_BUF[7], "_S_?>") != NULL)
		{
			device_status_get();
			UART1Write_Str((uint8_t *)"AT+CMGDA=\"DEL ALL\"\r\n");
		    delay_nms(100);
			 GSM_BUSY = 0;
			return 1;
		}
		/***转换成有效指令***/
		i=0;
		while(sim800c_RCV_BUF[i+7] != 0)
		{
			cmd_temp[i] = sim800c_RCV_BUF[i+7];
			i++;
			if(i>71)
				break;
		}
		cmd_temp[i+1] = '\0';
		
		sprintf((char *)sim800c_RCV_BUF, "<BP%02u%s",EPROM.address, cmd_temp);
		GSM_CMD_FLAG = 1;
	#ifdef USE_DEBUG	
		UART0Write_Str(sim800c_RCV_BUF);
		delay_nms(100);
	#endif
		GSM_BUSY = 0;
		Cmd_process((char *)sim800c_RCV_BUF);
		
		GSM_BUSY = 0;
		UART1Write_Str((uint8_t *)"AT+CMGDA=\"DEL ALL\"\r\n");
		delay_nms(100);
		
	}
	return 0;
}


/*********************************************************************************************************
** 函数名称: device_status_get
** 功能描述: 获取设备状态
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void device_status_get(void)
{
	uint8_t  i;
	uint8_t  power_temp[6];
	uint16_t power_temp16;
	uint8_t ph[29] = {0};
	
	/***初始化第一部分信息***/
	sprintf((char *)GSM_MSG_BUF, "668F6F4EF67248672C00560032002E00310033000D000A");
	/***电源状态***/
  POWER_STAT = GET_POWER_STAT;
	if(POWER_STAT == 0) //电源正常
		strcat((char *)GSM_MSG_BUF, "75356E906B635E38000D000A");
	else
		strcat((char *)GSM_MSG_BUF, "72356E905F025E38000D000A");
	
	/***Rx告警功率***/
	for(i=0;i<CHANNEL_NUM*2;i++)
	{
		if(EPROM.q_power[i] >= 0)
		{
			power_temp[0] = 'B';
			power_temp16 =  EPROM.q_power[i]*100;
		}
		else
		{
			power_temp[0] = 'D';
			power_temp16 = (0 - EPROM.q_power[i])*100;
		}
		power_temp[1] = power_temp16/1000;
		power_temp[2] = power_temp16%1000/100;
		power_temp[3] = power_temp16%100/10;
		power_temp[4] = power_temp16%10;
		power_temp[5] = '\0';
		sprintf((char *)&MSG_q_power[i][0], "0052003%1d544A8B66529F7387002%1c003%1d003%1d002E003%1d003%1d00640042006D000D000A",
						 i+1, power_temp[0], power_temp[1], power_temp[2], power_temp[3], power_temp[4]
					 );
		strcat((char *)GSM_MSG_BUF, (char *)&MSG_q_power[i][0]);
	}
	/***发送状态短信第一部分***/ 
    sim800c_send_to(GSM_PHONE_temp, GSM_MSG_BUF, 204);
	delay_nms(100);
	
	sim800c_send_to(GSM_PHONE_temp, ph, 20);

	/***初始化第二部分信息***/
	sprintf((char *)GSM_MSG_BUF, "8C");
	/***Rx工作波长***/
	for(i=0;i<CHANNEL_NUM*2;i++)
	{
	  if(EPROM.wavelength[i] == 0)
		{
		  sprintf((char *)&MSG_WAVELEN[i][0], "0052003%1d5DE54F5C6CE2957F0031003300310030006E006D000D000A",i+1);
			strcat((char *)GSM_MSG_BUF, (char *)&MSG_WAVELEN[i][0]);
		}
		else if(EPROM.wavelength[i] == 1)
		{
		  sprintf((char *)&MSG_WAVELEN[i][0], "0052003%1d5DE54F5C6CE2957F0031003500350030006E006D000D000A",i+1);
			strcat((char *)GSM_MSG_BUF, (char *)&MSG_WAVELEN[i][0]);
		}
	}
	/***Rx状态***/
	for(i=0;i<CHANNEL_NUM*2;i++)
	{
		if(warn[i] == 0) //探光正常
		{
			if(power_warn[i] >= 0)
			{
				power_temp[0] = 'B';
				power_temp16 =  power[i]*100;
			}
			else
			{
				power_temp[0] = 'D';
				power_temp16 = (0 - power_warn[i])*100;
			}
			power_temp[1] = power_temp16/1000;
			power_temp[2] = power_temp16%1000/100;
			power_temp[3] = power_temp16%100/10;
			power_temp[4] = power_temp16%10;
			power_temp[5] = '\0';

			sprintf((char *)&MSG_CHANNEL_STAT[i][0], "0052003%1d6B635E38FF0C5F53524D5149529F7387002%1c003%1d003%1d002E003%1d003%1d00640042006D000D000A",
							 i+1, power_temp[0], power_temp[1], power_temp[2], power_temp[3], power_temp[4]
						 );
			strcat((char *)GSM_MSG_BUF, (char *)&MSG_CHANNEL_STAT[i][0]);
		}
		else
		{
			if(power_warn[i] >= 0)
			{
				power_temp[0] = 'B';
				power_temp16 =  power_warn[i]*100;
			}
			else
			{
				power_temp[0] = 'D';
				power_temp16 = (0 - power_warn[i])*100;
			}
			power_temp[1] = power_temp16/1000;
			power_temp[2] = power_temp16%1000/100;
			power_temp[3] = power_temp16%100/10;
			power_temp[4] = power_temp16%10;
			power_temp[5] = '\0';

			sprintf((char *)&MSG_CHANNEL_STAT[i][0], "0052003%1d5F025E38FF0C5F53524D5149529F7387002%1c003%1d003%1d002E003%1d003%1d00640042006D000D000A",
							 i+1, power_temp[0], power_temp[1], power_temp[2], power_temp[3], power_temp[4]
						 );
			strcat((char *)GSM_MSG_BUF, (char *)&MSG_CHANNEL_STAT[i][0]);
		}
	}
	/***发送状态短信第二部分***/
	sim800c_send_to(GSM_PHONE_temp, GSM_MSG_BUF, 280);
    delay_nms(100);
	sim800c_send_to(GSM_PHONE_temp, ph, 20);
	/***初始化第三部分信息***/
	sprintf((char *)GSM_MSG_BUF, "%2X", (76*EPROM.PHONE_NUM)/2);
	for(i=0;i<EPROM.PHONE_NUM;i++)
	{
	  sprintf((char *)&MSG_PHONE[i][0], "544A8B6653F77801003%1dFF1A003%1c003%1c003%1c003%1c003%1c003%1c003%1c003%1c003%1c003%1c003%1c000D000A",
			       i+1, EPROM.PHONE[i][0], EPROM.PHONE[i][1], EPROM.PHONE[i][2], EPROM.PHONE[i][3], EPROM.PHONE[i][4],
              		EPROM.PHONE[i][5], EPROM.PHONE[i][6], EPROM.PHONE[i][7], EPROM.PHONE[i][8], EPROM.PHONE[i][9], EPROM.PHONE[i][10]
		       );
		strcat((char *)GSM_MSG_BUF, (char *)&MSG_PHONE[i][0]);
	}
	
	/***发送状态短信第三部分***/
	sim800c_send_to(GSM_PHONE_temp, GSM_MSG_BUF, 76*EPROM.PHONE_NUM);
}

/*********************************************************************************************************
** 函数名称: device_status_check
** 功能描述: 检查设备状态
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void device_status_check(void)
{
	uint8_t  i;

	if(ALM_FLAG == 0)
	{
		/***电源状态***/
		POWER_STAT = GET_POWER_STAT;
		if(POWER_STAT != POWER_STAT_OLD)
		{
		    PWR_ALM_FLAG = 1;
			ALM_FLAG = 1;
		}
		else
			PWR_ALM_FLAG = 0;
		POWER_STAT_OLD = POWER_STAT;

		/***Rx状态***/
		for(i=0;i<CHANNEL_NUM*2;i++)
		{
			if(CHANNEL_STAT_OLD[i] != warn[i])
			{
			    ALM_FLAG = 1;
				CH_ALM_FLAG = 1;
			}
			else
			  CH_ALM_FLAG = 0;
			CHANNEL_STAT_OLD[i] = warn[i];
		}		
	}
}
/*********************************************************************************************************
** 函数名称: GSM_TCPC_INIT
** 功能描述: GSM TCP客户端初始化
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
void GSM_ALM(void)
{
	char *cjson_msg;
	uint8_t  i;
	uint8_t  data[100];
	uint8_t  power_temp[7];
	uint16_t power_temp16;
	uint16_t j=0;
	LOG_DATA log_data;
	
	/***当状态发生变化时告警***/
	if(ALM_FLAG == 1)
	{
		/***日志记录***/
		LOG_WRITE(&log_data);
		
    /***短信告警功能***/
		if(EPROM.SMS_ALT)
		{
			/***SMS 信息头***/
			sprintf((char *)GSM_MSG_BUF, "58");
			
			if(POWER_STAT == 0) //电源正常
				strcat((char *)GSM_MSG_BUF, "75356E906B635E38");
			else
				strcat((char *)GSM_MSG_BUF, "75356E905F025E38");

			for(i=0;i<CHANNEL_NUM*2;i++)
			{
				power_temp[0] = i + 1;
				if(CHANNEL_STAT_OLD[i] == 0) //探光正常
				{
					if(power_warn[i] >= 0)
					{
						power_temp[1] = 'B';
						power_temp16 =  power_warn[i]*100;
					}
					else
					{
						power_temp[1] = 'D';
						power_temp16 = (0 - power_warn[i])*100;
					}
					power_temp[2] = power_temp16/1000;
					power_temp[3] = power_temp16%1000/100;
					power_temp[4] = power_temp16%100/10;
					power_temp[5] = power_temp16%10;
					power_temp[6] = '\0';

					sprintf((char *)&MSG_CHANNEL[i][0], "FF0C0052003%1d6B635E38FF0C5F53524D5149529F7387002%1c003%1d003%1d002E003%1d003%1d00640042006D",
									 power_temp[0], power_temp[1], power_temp[2], power_temp[3], power_temp[4], power_temp[5]
								 );
					strcat((char *)GSM_MSG_BUF, (char *)&MSG_CHANNEL[i][0]);
				}
				else
				{
					if(power_warn[i] >= 0)
					{
						power_temp[1] = 'B';
						power_temp16 =  power_warn[i]*100;
					}
					else
					{
						power_temp[1] = 'D';
						power_temp16 = (0 - power_warn[i])*100;
					}
					power_temp[2] = power_temp16/1000;
					power_temp[3] = power_temp16%1000/100;
					power_temp[4] = power_temp16%100/10;
					power_temp[5] = power_temp16%10;
					power_temp[6] = '\0';

					sprintf((char *)&MSG_CHANNEL[i][0], "FF0C0052003%1d5F025E38FF0C5F53524D5149529F7387002%1c003%1d003%1d002E003%1d003%1d00640042006D",
									 power_temp[0], power_temp[1], power_temp[2], power_temp[3], power_temp[4], power_temp[5]
								 );
					strcat((char *)GSM_MSG_BUF, (char *)&MSG_CHANNEL[i][0]);
				}
			}
			sim800c_send(GSM_MSG_BUF, 176);
		}
		
		if(GSM_BUSY == 0)
		{
       GSM_BUSY = 1;
		  
			/***检测网络状态***/
			UART1Write_Str((uint8_t *)"AT+CIPSTATUS\r\n");
			j=0;
			do
			{
				data[0] = UART1Get();
				j++;
				if(j >= 20000)
				{
					UART0Write_Str((uint8_t *)"GSM/TCP STATUS ERROR\n");
					GSM_BUSY = 0;
					return;
				}
			}while( data[0] != ':');
			for(i=1;i<100;i++)
				data[i] = UART1Get();
			if((strstr((char *)data, "INITIAL")) == NULL)
			{
				UART1Write_Str(GSM_CIPCLOSE);
				OSTimeDly(100);
				UART1Write_Str(GSM_CIPSHUT);
				UART0Write_Str((uint8_t *)"GSM/TCP INIT FAIL\n");
				GSM_BUSY = 0;
				return;
			}
			UART1Write_Str(GSM_BUF6);
			OSTimeDly(100);
			UART1Write_Str(GSM_BUF7);
			while((data[0] = UART1Get()) != 'O')/* empty loop */;
			for(i=1;i<2;i++)
				data[i] = UART1Get();
			if(strstr((char*)data, "OK") != NULL)
			{
				UART1Write_Str(GSM_BUF8);
				OSTimeDly(100);
				UART1Write_Str(GSM_BUF9);
				j=0;
				do
				{
					data[0] = UART1Get();
					j++;
					if(j >= 20000)
					{
						UART1Write_Str(GSM_CIPSHUT);
						UART0Write_Str((uint8_t *)"CONNECT TIMEOUT\n");
						GSM_BUSY = 0;
						return;
					}
				}while( data[0] !=  'N' );
				j=0;
				for(i=1;i<15;i++)
					data[i] = UART1Get();
				if(strstr((char*)data, "NNECT OK") != NULL)
				{
					if(PWR_ALM_FLAG)
						cjson_msg = CreateINFOCommand();
					else if(CH_ALM_FLAG)
						cjson_msg = CreateBACommand();
					UART1Write_Str(GSM_CIPSEND);
					OSTimeDly(100);
					UART1Write_Str((uint8_t *)cjson_msg);
					delay_nms(10);
					UART1Putch(GSM_MSG_STOP_FLAG);
					OSTimeDly(5000);
					UART1Write_Str(GSM_CIPCLOSE);
					OSTimeDly(100);
					UART1Write_Str(GSM_CIPSHUT);
				}
				else
				{
					UART1Write_Str(GSM_CIPSHUT);
					UART0Write_Str((uint8_t *)"CONNECT FAIL\n");
				}
			}
			else
			{
				UART1Write_Str(GSM_CIPCLOSE);
				OSTimeDly(100);
				UART1Write_Str(GSM_CIPSHUT);
				UART0Write_Str((uint8_t *)"GSM/TCP INIT FAIL\n");
			}
			GSM_BUSY = 0;
		}
		
		ALM_FLAG = 0;
		GSM_BUSY = 0;
	}
}




char * CreateBACommand()
{
	uint8 i;
	int wl[2];
	cJSON *root;
	char *rendered;
	cJSON *data;
	cJSON *r1, *r2;
/*	char temp[7] ;
	uint16_t data_temp16;
	float tt = -12.34;
    */
	for(i=0; i<2; i++)
	{
		if(EPROM.wavelength[i])
			wl[i] = 1550;
		else
			wl[i] = 1310;	
	}
	// CREATE BA COMMAND
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "ba");
	data = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "data", data);

	cJSON_AddItemToArray(data, r1 = cJSON_CreateObject());
	cJSON_AddStringToObject(r1, "name", "R1");
	
	cJSON_AddNumberToObject(r1, "wavelength", wl[0]);
/*    if (power[0] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  power[0] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - power[0]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r1,"powerv", temp);*/
    
	cJSON_AddNumberToObject(r1, "powerv", power[0]);

	cJSON_AddItemToArray(data, r2 = cJSON_CreateObject());
	cJSON_AddStringToObject(r2, "name", "R2");
	cJSON_AddNumberToObject(r2, "wavelength", wl[1]);
/*(    if (power[1] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  power[1] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - power[1]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r2,"powerv", temp);*/
	cJSON_AddNumberToObject(r2, "powerv", power[1]);

	rendered = cJSON_PrintUnformatted(root);

	cJSON_Delete(root);

	return rendered;
}



char * CreateRXWCommand()
{
	uint8 i;
	int wl[2];
	cJSON *root;
	char *rendered;
	cJSON *data;
	cJSON *r1, *r2;
	char temp[7] ;
	uint16_t data_temp16;


	for(i=0; i<2; i++)
	{
		if(EPROM.wavelength[i])
			wl[i] = 1550;
		else
			wl[i] = 1310;	
	}
	// CREATE BA COMMAND
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "rxw");
	data = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "data", data);

	cJSON_AddItemToArray(data, r1 = cJSON_CreateObject());
	cJSON_AddStringToObject(r1, "name", "R1");
	cJSON_AddNumberToObject(r1, "wavelength",wl[0]);
	cJSON_AddNumberToObject(r1, "powerv", EPROM.q_power[0]);

/*	if (EPROM.q_power[0] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  EPROM.q_power[0] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - EPROM.q_power[0]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r1,"powerv", temp);*/
	
	cJSON_AddItemToArray(data, r2 = cJSON_CreateObject());
	cJSON_AddStringToObject(r2, "name", "R2");
	cJSON_AddNumberToObject(r2, "wavelength",wl[1]);

/*    if (EPROM.q_power[1] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  EPROM.q_power[1] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - EPROM.q_power[1]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r1,"powerv", temp);*/
	cJSON_AddNumberToObject(r2, "powerv", EPROM.q_power[1]);
	rendered = cJSON_PrintUnformatted(root);

	cJSON_Delete(root);

	return rendered;
}




char * CreateRXPCommand()
{
	uint8 i;
	int wl[2];
	cJSON *root;
	char *rendered;
	cJSON *data;
	cJSON *r1, *r2;
	char temp[7] ;
	uint16_t data_temp16;


	for(i=0; i<2; i++)
	{
		if(EPROM.wavelength[i])
			wl[i] = 1550;
		else
			wl[i] = 1310;	
	}
	// CREATE BA COMMAND
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "rxp");
	data = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "data", data);

	cJSON_AddItemToArray(data, r1 = cJSON_CreateObject());
	cJSON_AddStringToObject(r1, "name", "R1");
	//cJSON_AddNumberToObject(r1, "wavelength",wl[0]);
	cJSON_AddNumberToObject(r1, "threshold", EPROM.q_power[0]);

/*	if (EPROM.q_power[0] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  EPROM.q_power[0] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - EPROM.q_power[0]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r1,"powerv", temp);*/
	
	cJSON_AddItemToArray(data, r2 = cJSON_CreateObject());
	cJSON_AddStringToObject(r2, "name", "R2");
	//cJSON_AddNumberToObject(r2, "wavelength",wl[1]);

/*    if (EPROM.q_power[1] >= 0)
	{
			temp[0] = '+';
			data_temp16 =  EPROM.q_power[1] * 100;
	}
	else
	{
			temp[0] = '-';
			data_temp16 = (0 - EPROM.q_power[1]) * 100;
	}
	temp[1] = data_temp16 / 1000 + '0';
	temp[2] = data_temp16 % 1000 / 100 + '0';
	temp[3] = '.';
	temp[4] = data_temp16 % 100 / 10 + '0';
	temp[5] = data_temp16 % 10 + '0';
	temp[6] = '\0';
	cJSON_AddStringToObject(r1,"powerv", temp);*/
	cJSON_AddNumberToObject(r2, "threshold", EPROM.q_power[1]);
	rendered = cJSON_PrintUnformatted(root);

	cJSON_Delete(root);

	return rendered;
}





char * CreateINFOCommand(void)
{
	cJSON *root;
	char *rendered;
	cJSON *data;

	POWER_STAT = GET_POWER_STAT;
	// CREATE INFO COMMAND
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "info");

	cJSON_AddItemToObject(root, "data", data = cJSON_CreateObject());
	cJSON_AddStringToObject(data, "version", "2.13");
	cJSON_AddNumberToObject(data, "power", !POWER_STAT);
	cJSON_AddNumberToObject(data, "gsm", 0);

	rendered = cJSON_PrintUnformatted(root);

	cJSON_Delete(root);
	
	return rendered;
}

char * CreateQBWCommand(void)
{
	cJSON *root;
	char *rendered;

	// CREATE QBW COMMAND
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "qbw");

	rendered = cJSON_Print(root);
	cJSON_Delete(root);
	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "pver", "1");
	cJSON_AddStringToObject(root, "uid", EPROM.GUID);
	cJSON_AddStringToObject(root, "cmd", "qbw");

	rendered = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
//#ifdef USE_DEBUG
//    UART0Write_Str((uint8_t *)rendered);
//#endif
	return rendered;
}



/*********************************************************************************************************
** 函数名称: GSM_TCPC
** 功能描述: GSM TCP客户端
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
int GSM_TCPC(void)
{
	static int err_count = 0;
	uint8_t  i, bracer, bracel;
	uint8_t  data[101] = {0};
	uint16_t j=0,json_s = 0;
	char *cp, *cjson_msg, *citem;
	cJSON *msg, *key, *array, *item;
	volatile int temp = 0;
	   memset(data, 0, 100);
	while(GSM_BUSY == 1)
	{
		OSTimeDly(2);
	}
	 if(GSM_BUSY == 0)
	 {
			    GSM_BUSY = 1;
			    if(check_ststus(GSM_BUF6,"OK",2,5000,0) < 0)
				{
					err_count ++;
				}
				if((temp = check_ststus(GSM_BUF7,"OK",2,5000,0)) < 0)
				{
					err_count ++;
				}
				if(err_count > RESET_COUNT)
				{
					err_count = 0;
					if(EPROM.use_TCP == 1)
					{
						 PWRKEY_L;
						 OSTimeDly(2000);
						 PWRKEY_H;
						 while(SIM_STA)
						 {
							 OSTimeDly(1);
						 }
						 OSTimeDly(1000);
						 PWRKEY_L;
						 OSTimeDly(2000);
						 PWRKEY_H;
					}
					
				}
			    if(temp > 0)
				{
					err_count  =0;
						UART1Write_Str(GSM_BUF8);
						OSTimeDly(200);
					    temp = 0;
						temp = check_ststus(GSM_BUF9,"CONNECT OK",10,3000,0);
					    if(temp < 0)
						{
							UART1Write_Str(GSM_CIPSHUT);
							UART0Write_Str((uint8_t *)"CONNECT TIMEOUT\n");
							GSM_BUSY = 0;
							return -1;
						}
						if(temp >= 0)
						{
							
							if(reboot >= 12 )
							{
								PWRKEY_L;
								OSTimeDly(2000);
								PWRKEY_H;
								while(SIM_STA)
								{
								 OSTimeDly(1);
								}
								OSTimeDly(1000);
								PWRKEY_L;
								OSTimeDly(2000);
								PWRKEY_H;		
								Reset_Handler(); 
							}
							
							    memset(GSM_RCV_BUF, 0, 1023);
							
								cjson_msg = CreateINFOCommand();
								UART1Write_Str(GSM_CIPSEND);
								OSTimeDly(100);
								UART1Write_Str((uint8_t *)cjson_msg);
								delay_nms(100);
							    free((char *)cjson_msg);
								UART1Putch(GSM_MSG_STOP_FLAG);
							    temp = 0;
                                temp  = deal_string("RECV FROM:",10,15000);
								if(temp < 0 )
								{
									UART1Write_Str(GSM_CIPCLOSE);
									OSTimeDly(100);
									UART1Write_Str(GSM_CIPSHUT);
									UART0Write_Str((uint8_t *)"NO RESP\n");
									GSM_BUSY = 0;
									reboot++;
									OSTimeDly(100);
									return -1;
								}
 
							    
                                OSTimeDly(100);
								cjson_msg = CreateBACommand();
								UART1Write_Str(GSM_CIPSEND);
								OSTimeDly(100);
								UART1Write_Str((uint8_t *)cjson_msg);
								delay_nms(100);
								free((char *)cjson_msg);
								UART1Putch(GSM_MSG_STOP_FLAG);
                               	temp = 0;
                                temp  = deal_string("RECV FROM:",10,15000);
								if(temp < 0 )
								{
									UART1Write_Str(GSM_CIPCLOSE);
									OSTimeDly(100);
									UART1Write_Str(GSM_CIPSHUT);
									UART0Write_Str((uint8_t *)"NO RESP\n");
									GSM_BUSY = 0;
									reboot++;
									OSTimeDly(100);
									return -1;
								}
								OSTimeDly(100);
								cjson_msg = CreateQBWCommand();
								UART1Write_Str(GSM_CIPSEND);
								OSTimeDly(100);
								UART1Write_Str((uint8_t *)cjson_msg);
								delay_nms(100);
								free((char *)cjson_msg);
								UART1Putch(GSM_MSG_STOP_FLAG);
								
								temp = 0;
                                temp  = deal_string("RECV FROM:",10,15000);
								if(temp < 0 )
								{
									UART1Write_Str(GSM_CIPCLOSE);
									OSTimeDly(100);
									UART1Write_Str(GSM_CIPSHUT);
									UART0Write_Str((uint8_t *)"NO RESP\n");
									GSM_BUSY = 0;
									reboot++;
									OSTimeDly(100);
									return -1;
								}
								temp = get_string(GSM_RCV_BUF,(GSM_BUF_SIZE-2),3000);
								OSTimeDly(10);
								if(temp < 0)
								{
									UART1Write_Str(GSM_CIPCLOSE);
									OSTimeDly(100);
									UART1Write_Str(GSM_CIPSHUT);
									UART0Write_Str((uint8_t *)"NO RESP1\n");
									GSM_BUSY = 0;
									OSTimeDly(100);
									return -1;
								}
								reboot = 0;
					#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"recieve OK !!!\n");
								UART0Write_Str(GSM_RCV_BUF);
								OSTimeDly(10);
					 #endif
								FeedDog();	
								j = 0;
								json_s = 0;
                                while(GSM_RCV_BUF[j]!= '{')
								{
									j++;
									json_s++;
										if(j >= (GSM_BUF_SIZE-2) )
										{
												UART1Write_Str(GSM_CIPCLOSE);
												OSTimeDly(100);
												UART1Write_Str(GSM_CIPSHUT);
												UART0Write_Str((uint8_t *)"RESP TIMEOUT\n");
												GSM_BUSY = 0;
											    OSTimeDly(100);
												return -1;
										}
								}
								j++;
					 #ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"NO TIMEOUT\r\n");
								OSTimeDly(10);
					 #endif
								bracel = 1;
								bracer = 0;
								while(1)
								{
										if(GSM_RCV_BUF[j] == '{')
												bracel++;
										else if(GSM_RCV_BUF[j] == '}')
												bracer++;
										if(bracel == bracer)
										{
											GSM_RCV_BUF[j+1] = '\0';
											break;
										}
												
										j++;
										if(j >= GSM_BUF_SIZE)
										{
												UART0Write_Str((uint8_t *)"NO RESP\n");
											    UART1Write_Str(GSM_CIPCLOSE);
								                OSTimeDly(100);
								                UART1Write_Str(GSM_CIPSHUT);
					  						    GSM_BUSY = 0;
											    OSTimeDly(100);
												return -1;
										}
								}
							    FeedDog();
								msg = cJSON_Parse((char *)&GSM_RCV_BUF[json_s]);
								cp = cJSON_PrintUnformatted(cJSON_GetObjectItem(msg, "cmd"));
					#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)&GSM_RCV_BUF[json_s]);
								OSTimeDly(10);
					#endif      
								if((strstr(cp, "qbw")) != NULL)
								{
					#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"read qbw OK\r\n");
									OSTimeDly(10);
					#endif
								    cp = cJSON_PrintUnformatted(cJSON_GetObjectItem(msg, "msg"));
									
									  if((strstr(cp, "Success")) != NULL)
										{
					#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"read Success OK\r\n");
								OSTimeDly(10);
					#endif
												key = cJSON_GetObjectItem(msg, "result");
												cp = cJSON_PrintUnformatted(key);
												array = cJSON_Parse(cp);
												
												for(i=0; i<2; i++)
												{
									                    
														item = cJSON_GetArrayItem(array, i);
													  key = cJSON_GetObjectItem(item, "threshold");
														citem = cJSON_PrintUnformatted(item);
														if((strstr(citem, "1310")) != NULL)
														{
															EPROM.wavelength[i] = 0;
															Save_To_EPROM((uint8 *)&EPROM.wavelength[i], 1);
														}else if((strstr(citem, "1550")) != NULL)
														{
															EPROM.wavelength[i] = 1;
															Save_To_EPROM((uint8 *)&EPROM.wavelength[i], 1);
														}else
														{
																UART0Write_Str((uint8*)"PARAMETERw UPDATA ERROR");
															    OSTimeDly(10);
														}
														EPROM.q_power[i] = key->valuedouble;
														sprintf((char *)data,"%d.%02u",(int)EPROM.q_power[i],(unsigned int)(EPROM.q_power[i]*100)%100);
														Save_To_EPROM((uint8 *)&EPROM.q_power[i], 4);
														/*if((ctemp = strstr(citem, "-")) != NULL || (ctemp = strstr(citem, "+")) != NULL)
														{
															
															data_temp8 = 0;
															data_temp[0] = 0;
															data_temp[1] = 0;
															
															strncpy(cdata,ctemp,6);
															
															data_temp8 = ((cdata[1]-'0')*10) + (cdata[2]-'0');
															if(data_temp8 <= 99)
																data_temp[0]=data_temp8;
															data_temp8 = ((cdata[4]-'0')*10) + (cdata[5]-'0');
															if(data_temp8 <= 99)
																data_temp[1]=data_temp8;
															if((data_temp[0] <= 99) &&(data_temp[1] <= 99))
															{
																EPROM.q_power[i] = -(data_temp[0]+(data_temp[1]*0.01));
																Save_To_EPROM((uint8 *)&EPROM.q_power[i], 4);
															}
														#ifdef USE_DEBUG	
															sprintf((char *)data,"R%d==qw==%6.2f",i,(float)EPROM.q_power[i]);
															UART0Write_Str(data);
															OSTimeDly(10);
														#endif
														}
														else
														{
															UART0Write_Str((uint8*)"PARAMETERp UPDATA ERROR2");
															OSTimeDly(10);
														}*/
													#ifdef USE_DEBUG		 
														UART0Write_Str((uint8*)data);
														OSTimeDly(10);
													#endif
												}
										}
										free((char *)cp);
										
								}
								
								cJSON_Delete(msg);
								
								UART1Write_Str(GSM_CIPCLOSE);
								OSTimeDly(100);
								UART1Write_Str(GSM_CIPSHUT);
								OSTimeDly(10);
						}
						else
						{
								UART1Write_Str(GSM_CIPSHUT);
							    OSTimeDly(10);
								UART0Write_Str((uint8_t *)"CONNECT FAIL\n");
							    OSTimeDly(10);
						}
				}
				else
				{
						UART1Write_Str(GSM_CIPCLOSE);
						OSTimeDly(100);
						UART1Write_Str(GSM_CIPSHUT);
						UART0Write_Str((uint8_t *)"GSM/TCPC INIT FAIL\n");
					    OSTimeDly(2000);
				}
				GSM_BUSY = 0;

		}
       return 0;

}

/*********************************************************************************************************
** 函数名称: GSM_SMS_TCPC
** 功能描述: 短息设置更新服务器客户端
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
int GSM_SMS_W_TCPC(int *ret)
{
	
		uint8_t  bracer, bracel;
		uint8_t  data[200];
		uint16_t j=0,json_s = 0;
		char *cp, *cjson_msg;
		cJSON *msg;
		int temp = 0;
		int retu =  0;
		if(check_ststus(GSM_BUF6,"OK",2,5000,0) < 0)
		{
			err_count1 ++;
		}
		if((temp = check_ststus(GSM_BUF7,"OK",2,5000,0)) < 0)
		{
			err_count1 ++;
		}
		if(err_count1 > RESET_COUNT)
		{
			err_count1 = 0;
			if(EPROM.use_TCP == 1)
			{
				 PWRKEY_L;
				 OSTimeDly(2000);
				 PWRKEY_H;
				 while(SIM_STA)
				 {
					 OSTimeDly(1);
				 }
				 OSTimeDly(1000);
				 PWRKEY_L;
				 OSTimeDly(2000);
				 PWRKEY_H;
			}
		}
		if(temp >= 0)
		{
				err_count1  =0;
				UART1Write_Str(GSM_BUF8);
				OSTimeDly(200);
				temp = 0;
				temp = check_ststus(GSM_BUF9,"CONNECT OK",10,3000,0);
				if(temp < 0)
				{
					UART1Write_Str(GSM_CIPSHUT);
					UART0Write_Str((uint8_t *)"CONNECT TIMEOUT\n");
					*ret = -1;
					return retu;
				}
				else if(temp >= 0)
				{
				
						memset(data, 0, 198);
						OSTimeDly(100);
						cjson_msg = CreateRXWCommand();
						UART1Write_Str(GSM_CIPSEND);
						OSTimeDly(100);
						UART1Write_Str((uint8_t *)cjson_msg);
						delay_nms(100);
						free((char *)cjson_msg);
						UART1Putch(GSM_MSG_STOP_FLAG);				                
						OSTimeDly(5);
						temp = 0;
						//temp  = deal_string("RECV FROM:",10,7000);
					     temp  = deal_string("Success",7,7000);
						if(temp < 0 )
						{
							UART1Write_Str(GSM_CIPCLOSE);
							OSTimeDly(100);
							UART1Write_Str(GSM_CIPSHUT);
							UART0Write_Str((uint8_t *)"NO RESP\n");
							*ret = -1;
							return retu;
						}
						retu = 1;
						*ret = 0;
						UART1Write_Str(GSM_CIPCLOSE);
						OSTimeDly(100);
						UART1Write_Str(GSM_CIPSHUT);
						OSTimeDly(10);
				}
				else
				{
						UART1Write_Str(GSM_CIPSHUT);
						OSTimeDly(10);
						UART0Write_Str((uint8_t *)"CONNECT FAIL\n");
						OSTimeDly(10);
						*ret = -1;
				}
		}
		else
		{
				UART1Write_Str(GSM_CIPCLOSE);
				OSTimeDly(100);
				UART1Write_Str(GSM_CIPSHUT);
				UART0Write_Str((uint8_t *)"GSM/TCPC INIT FAIL\n");
				OSTimeDly(2000);
				*ret = -1;
		}


		return retu;
}




























/*********************************************************************************************************
** 函数名称: GSM_SMS_TCPC
** 功能描述: 短息设置更新服务器客户端
** 输　入: 无
** 输　出: 无
********************************************************************************************************/
int GSM_SMS_P_TCPC(int *ret)
{
	
		uint8_t  bracer, bracel;
		uint8_t  data[200];
		uint16_t j=0,json_s = 0;
		char *cp, *cjson_msg;
		cJSON *msg;
		int temp = 0;
		int retu =  0;
		if(check_ststus(GSM_BUF6,"OK",2,5000,0) < 0)
		{
			err_count1 ++;
		}
		if((temp = check_ststus(GSM_BUF7,"OK",2,5000,0)) < 0)
		{
			err_count1 ++;
		}
		if(err_count1 > RESET_COUNT)
		{
			err_count1 = 0;
			if(EPROM.use_TCP == 1)
			{
				 PWRKEY_L;
				 OSTimeDly(2000);
				 PWRKEY_H;
				 while(SIM_STA)
				 {
					 OSTimeDly(1);
				 }
				 OSTimeDly(1000);
				 PWRKEY_L;
				 OSTimeDly(2000);
				 PWRKEY_H;
			}
		}
		if(temp >= 0)
		{
				err_count1  =0;
				UART1Write_Str(GSM_BUF8);
				OSTimeDly(200);
				temp = 0;
				temp = check_ststus(GSM_BUF9,"CONNECT OK",10,3000,0);
				if(temp < 0)
				{
					UART1Write_Str(GSM_CIPSHUT);
					UART0Write_Str((uint8_t *)"CONNECT TIMEOUT\n");
					*ret = -1;
					return retu;
				}
				else if(temp >= 0)
				{
				
						memset(data, 0, 198);
						OSTimeDly(100);
						cjson_msg = CreateRXPCommand();
						UART1Write_Str(GSM_CIPSEND);
						OSTimeDly(100);
						UART1Write_Str((uint8_t *)cjson_msg);
						delay_nms(100);
						free((char *)cjson_msg);
						UART1Putch(GSM_MSG_STOP_FLAG);				                
						OSTimeDly(5);
						temp = 0;
						//temp  = deal_string("RECV FROM:",10,7000);
					    temp  = deal_string("Success",7,7000);
						if(temp < 0 )
						{
							UART1Write_Str(GSM_CIPCLOSE);
							OSTimeDly(100);
							UART1Write_Str(GSM_CIPSHUT);
							UART0Write_Str((uint8_t *)"NO RESP\n");
							*ret = -1;
							return retu;
						}
						retu = 1;
						*ret = 0;
						UART1Write_Str(GSM_CIPCLOSE);
						OSTimeDly(100);
						UART1Write_Str(GSM_CIPSHUT);
						OSTimeDly(10);
				}
				else
				{
						UART1Write_Str(GSM_CIPSHUT);
						OSTimeDly(10);
						UART0Write_Str((uint8_t *)"CONNECT FAIL\n");
						OSTimeDly(10);
						*ret = -1;
				}
		}
		else
		{
				UART1Write_Str(GSM_CIPCLOSE);
				OSTimeDly(100);
				UART1Write_Str(GSM_CIPSHUT);
				UART0Write_Str((uint8_t *)"GSM/TCPC INIT FAIL\n");
				OSTimeDly(2000);
				*ret = -1;
		}
		return retu;
}





























