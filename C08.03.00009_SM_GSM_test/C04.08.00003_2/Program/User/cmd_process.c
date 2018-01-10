/**********************************************************************************************************
*文件名：Cmd_process.c
*创建时间: 2012-10-18
*功能描述: 处理串口和TCP的设置和查询指令
*作   者 ：
*公   司 ：
**********************************************************************************************************/
#include "main.h"
#include "uart.h"
#include "uart0.h"
#include "lpc177x_8x_eeprom.h"
#include "ADC.h"
#include "log.h"
#include "sim800c.h"


uint16 Cmd_process( char* sprintf_buf )
{
	
    uint8  i, j, data_temp8, link_num;
    uint8  data_temp[14];
	volatile uint16 data_temp16;
	uint16_t sprintf_len = 0;
	
    if( strstr(&sprintf_buf[0],"<BP") != NULL )
    {
			//<BP_ADR_?>
			if( strstr((char*)&sprintf_buf[3],"_ADR_?>") != NULL )
			{
				sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_OK>\r\n",EPROM.address);
			}
				
			//<BP01_ADR_XX>  更改设备地址
			else if( sprintf_buf[3]==(EPROM.address/10+'0') && sprintf_buf[4]==(EPROM.address%10+'0') && sprintf_buf[5]=='_' )
			{
				if( ( strstr((char*)&sprintf_buf[6],"ADR_"))!=NULL && sprintf_buf[12]=='>' )
				{
					data_temp8 = ((sprintf_buf[10]-'0')*10) + (sprintf_buf[11]-'0');
					if(data_temp8 < 100)
					{
						EPROM.address = data_temp8;
						Save_To_EPROM((uint8 *)&EPROM.address, 1);
						sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_ADR_OK>\r\n", EPROM.address);
					}
					else  goto send_err;
				}
				
				//<BP01_PWD_123456> 
				else if( (strstr((char*)&sprintf_buf[6],"PWD"))!=NULL && (sprintf_buf[11] == '>' || sprintf_buf[16] == '>' ))
				{
					if(sprintf_buf[10] == '?')
						sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_GSM_PWD_%s>\r\n",EPROM.address, EPROM.GSM_PASSWORD);
					else
					{
						for(i=0;i<6;i++)
						{
							if((sprintf_buf[i+10] >= '0') && (sprintf_buf[i+10] <= 'Z'))
							  EPROM.GSM_PASSWORD[i] = sprintf_buf[i+10];
							else
							{
							  if(GSM_CMD_FLAG == 1)
								sim800c_send_to(GSM_PHONE_temp, GSM_MSG_ERR4, 24);
								sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_PWD_ER>\r\n",EPROM.address);	
								return sprintf_len;
							}
						}
						EPROM.GSM_PASSWORD[6] = '\0';
						Save_To_EPROM((uint8 *)&EPROM.GSM_PASSWORD, 7);
						if(GSM_CMD_FLAG)
						{
							sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_PWD,24);
							return 0;
						}
						else
						  sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_PWD_OK>\r\n",EPROM.address);							
					}
				}
				
				//<BP01_GSM_ADDR_1380XXXX500>  0891683108703705F0
				else if( (strstr((char*)&sprintf_buf[6],"GSM_ADDR"))!=NULL && (sprintf_buf[16] == '>' || sprintf_buf[26] == '>' ))
				{
					if(sprintf_buf[15] == '?')
						  sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_GSM_ADDR_%s>\r\n",EPROM.address, EPROM.GSM_ADDR);
					else
					{
						uint8_t EPROM_GSM_ADDR[12];
						
						EPROM.GSM_ADDR[0] = '0';
						EPROM.GSM_ADDR[1] = '8';
						EPROM.GSM_ADDR[2] = '9';
						EPROM.GSM_ADDR[3] = '1';
						EPROM.GSM_ADDR[4] = '6';
						EPROM.GSM_ADDR[5] = '8';
						
						for(i=0;i<11;i++)
						{
							EPROM_GSM_ADDR[i] = sprintf_buf[i+15];
						}
						EPROM_GSM_ADDR[11] = 'F';
						for(i=0;i<6;i++)
						{
							EPROM.GSM_ADDR[(2*i)+7] = EPROM_GSM_ADDR[(2*i)];
						}
						for(i=0;i<6;i++)
						{
							EPROM.GSM_ADDR[(2*i)+6] = EPROM_GSM_ADDR[(2*i)+1];
						}
						EPROM.GSM_ADDR[18] = '\0';
						if(GSM_CMD_FLAG)
						{
							//sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_P,32);
							return 0;
						}
						
						sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_GSM_ADDR_OK>\r\n",EPROM.address);							  
					}
				}
				 
				//<BP01_PHONE_1380XXXX500_1380XXXX500_1380XXXX500> 11000D91683137773383F1000800
				else if( (strstr((char*)&sprintf_buf[6],"PHONE_"))!=NULL && (sprintf_buf[13] == '>' || sprintf_buf[23] == '>' || sprintf_buf[35] == '>'|| sprintf_buf[47] == '>'))
				{
					uint8_t xxtemp[12];
					uint8_t slen = 0;
					if(sprintf_buf[12] == '?')
					{
						sprintf_len = sprintf((char *)sprintf_buf,"PHONE:");
						for(i=0;i<EPROM.PHONE_NUM;i++)
						{
							slen += sprintf((char *)xxtemp,"%s_",&EPROM.PHONE[i][0]);
							strcat(sprintf_buf,(char *)xxtemp);
						}
						sprintf_len +=slen;
						sprintf_len +=2;
						strcat(sprintf_buf,"\r\n");
					}
					else
					{
						if(sprintf_buf[23] == '>')
							EPROM.PHONE_NUM = 1;
						else if(sprintf_buf[35] == '>')
							EPROM.PHONE_NUM = 2;
						else if(sprintf_buf[47] == '>')
							EPROM.PHONE_NUM = 3;
						Save_To_EPROM((uint8 *)&EPROM.PHONE_NUM, 1);
						
						for(i=0;i<EPROM.PHONE_NUM;i++)
						{
							EPROM.GSM_PHONE[i][0] = '1';
							EPROM.GSM_PHONE[i][1] = '1';
							EPROM.GSM_PHONE[i][2] = '0';
							EPROM.GSM_PHONE[i][3] = '0';
							EPROM.GSM_PHONE[i][4] = '0';
							EPROM.GSM_PHONE[i][5] = 'D';
							EPROM.GSM_PHONE[i][6] = '9';
							EPROM.GSM_PHONE[i][7] = '1';
							EPROM.GSM_PHONE[i][8] = '6';
							EPROM.GSM_PHONE[i][9] = '8';
							
							EPROM.GSM_PHONE[i][22] = '0';
							EPROM.GSM_PHONE[i][23] = '0';
							EPROM.GSM_PHONE[i][24] = '0';
							EPROM.GSM_PHONE[i][25] = '8';
							EPROM.GSM_PHONE[i][26] = '0';
							EPROM.GSM_PHONE[i][27] = '0';
							EPROM.GSM_PHONE[i][28] = '\0';
							
							for(j=0;j<11;j++)
							{
								EPROM.PHONE[i][j] = sprintf_buf[(j+12)+(i*12)];
							}
							EPROM.PHONE[i][11] = 'F';
							EPROM.PHONE[i][12] = '\0';
							Save_To_EPROM((uint8 *)&EPROM.PHONE[i][0], 13);
							
							for(j=0;j<6;j++)
							{
								EPROM.GSM_PHONE[i][(2*j)+11] = EPROM.PHONE[i][(2*j)];
							}
							for(j=0;j<6;j++)
							{
								EPROM.GSM_PHONE[i][(2*j)+10] = EPROM.PHONE[i][(2*j)+1];
							}
							
							Save_To_EPROM((uint8 *)&EPROM.GSM_PHONE[i][0], 29);
						}
					
						if(GSM_CMD_FLAG)
						{
							sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_PHONE,32);
							return 0;
						}
						sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_PHONE_OK>\r\n",EPROM.address);
					}
				}
				
				//<BP01_SMS_ALT_?> GSM SMS告警功能
				else if( strstr((char*)&sprintf_buf[6],"SMS_ALT_")!=NULL  )
				{
					if(sprintf_buf[14] == '?')
					  sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_SMS_ALT_%01u>\r\n", EPROM.address, EPROM.SMS_ALT);
					else
					{
					  if(sprintf_buf[14] == '0')
						{
							EPROM.SMS_ALT = 0;
						  Save_To_EPROM(&EPROM.SMS_ALT, 1);
							if(GSM_CMD_FLAG)
							{
							  sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_P,32);
								return 0;
							}
							sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_SMS_ALT_OK>\n", EPROM.address);
						}
						else if(sprintf_buf[14] == '1')
						{
							EPROM.SMS_ALT = 1;
						  Save_To_EPROM(&EPROM.SMS_ALT, 1);
							if(GSM_CMD_FLAG)
							{
							  sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_P,32);
								return 0;
							}
							sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_SMS_ALT_OK>\n", EPROM.address);
						}
						else goto send_err;
					}
				}
				
/*				//<BP01_GSMS_?> GSM状态查询
				else if( strstr((char*)&sprintf_buf[5],"_GSMS_?>")!=NULL  )
				{
					sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_GSMR_OK>\r\n", EPROM.address);
				}*/
				
				//<BP01_GSMR> GSM复位
				else if( strstr((char*)&sprintf_buf[6],"GSMR>")!=NULL  )
				{
						//UART1Write_Str((uint8_t *)"AT+CPOWD=1\r");
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

					sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_GSMR_OK>\r\n", EPROM.address);
				}
				
				//<BP01_RESET> 设备复位
				else if( strstr((char*)&sprintf_buf[6],"RESET>")!=NULL  )
				{
				    //sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_RESET_OK>\r\n", EPROM.address);
					//UART0Write_Str((uint8 *)sprintf_buf); 
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
				
				//<BP01_V_?> 查询模块软硬件信息
				else if( strstr((char*)&sprintf_buf[6],"V_?>")!=NULL )
				{
					sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_V%s>\r\n", EPROM.address, (char *)SVersion);
				}
				//<BP01_RX_W_Y> 设置波长 Y=0:1310nm,1:1550nm
				else if( (strstr(&sprintf_buf[8],"_W_"))!=NULL && sprintf_buf[12]=='>')
				{
					int t1 = 1;
					int t2 = 0;
					unsigned int t3 = 0;
					link_num = sprintf_buf[7]-'1';
					if( link_num<=( CHANNEL_NUM*2 -1 ) )
					{
						if(sprintf_buf[11] == '?')
						{
							sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_W_%c>\r\n", EPROM.address,sprintf_buf[7],(EPROM.wavelength[link_num]+'0'));
						}
                        else if(sprintf_buf[11] == '0' || sprintf_buf[11] == '1')
						{
							EPROM.wavelength[link_num] = sprintf_buf[11]-'0';
							Save_To_EPROM((uint8 *)&EPROM.wavelength[link_num], 1);
							
							sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_W_OK>\r\n", EPROM.address, sprintf_buf[7]);
                            if(EPROM.use_TCP == 1)
							{
								while(GSM_BUSY == 1)
								{
									OSTimeDly(1);
								}
								GSM_BUSY = 1;
								OSTimeDly(1000);
								OSTimeDly(100);
								while(t2 == 0)
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
									t2 = GSM_SMS_W_TCPC(&t1);
									OSTimeDly(2000);
								}
								if(GSM_CMD_FLAG)
								{
									GSM_CMD_FLAG = 0;
									sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_W,32);
								}
								OSTimeDly(100);
								GSM_BUSY = 0;
							}
						}else goto send_err;
					}
					else goto send_err;
				}
				
				//<BP01_RX_P_XXX.XX> 设置光功率阈值
				else if( (strstr(&sprintf_buf[8],"_P_"))!=NULL && (sprintf_buf[17]=='>' || sprintf_buf[12]=='>'))
				{
					int t1 = 1;
					int t2 = 0;
					unsigned int t3 = 0;
					link_num = sprintf_buf[7]-'1';
					
					if( (link_num<=( CHANNEL_NUM*2 - 1 )) )
					{
							
						if(sprintf_buf[11] == '-' || sprintf_buf[11] == '+' )
						{
							data_temp8 = ((sprintf_buf[12]-'0')*10) + (sprintf_buf[13]-'0');
							if(data_temp8 <= 99)
								data_temp[0]=data_temp8;
							else
								sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_ER>\r\n", EPROM.address, sprintf_buf[7]);

							data_temp8 = ((sprintf_buf[15]-'0')*10) + (sprintf_buf[16]-'0');
							if(data_temp8 <= 99)
								data_temp[1]=data_temp8;
							else
								sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_ER>\r\n", EPROM.address, sprintf_buf[7]);
							if((data_temp[0] <= 99) &&(data_temp[1] <= 99))
							{
								if(sprintf_buf[11] == '-')
								{
									EPROM.q_power[link_num] = -(data_temp[0]+(data_temp[1]*0.01));
								}
								
							    Save_To_EPROM((uint8 *)&EPROM.q_power[link_num], 4);
							}
							if(EPROM.use_TCP == 1)
							{
								while(GSM_BUSY == 1)
								{
									OSTimeDly(1);
								}
								GSM_BUSY = 1;
						#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"send_SMS_start\r\n");
						#endif
								OSTimeDly(100);
								while(t2 == 0)
								{
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
									t2 = GSM_SMS_P_TCPC(&t1);
									delay_nms(2000);
								}
								OSTimeDly(1000);
								if(GSM_CMD_FLAG)
								{
									GSM_CMD_FLAG = 0;
									sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_P,32);
								}
						#ifdef USE_DEBUG
								UART0Write_Str((uint8_t *)"send_SMS\r\n");
						#endif
								OSTimeDly(100);
								GSM_BUSY = 0;
							}
							
							sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_OK>\r\n", EPROM.address, sprintf_buf[7]);
						}
						else goto send_err;
/*						else if(*cpdata1=='+')
						{
							data_temp8=change_ascii_date( cpdata1+1, 2, &err );
							if( data_temp8<=99 && err==0 )
							{
								data_temp[0]=data_temp8;
							}
							else
								sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_ER>\r\n", EPROM.address, sprintf_buf[7]);

							data_temp8=change_ascii_date( cpdata2, 2, &err );
							if( data_temp8<=99 && err==0 )
								data_temp[1]=data_temp8;
							else
								sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_ER>\r\n", EPROM.address, sprintf_buf[7]);

							EPROM.q_power[link_num] = data_temp[0]+(data_temp[1]*0.01);
							Save_To_EPROM((uint8 *)&EPROM.q_power[link_num], 4);
							
							if(GSM_CMD_FLAG)
							{
							  sim800c_send_to(GSM_PHONE_temp, GSM_MSG_SET_P,32);
								return 0;
							}
							sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_R%c_P_OK>\r\n", EPROM.address, sprintf_buf[7]);
						}*/
					
					
					}
					else goto send_err;
				}
				
				//<BP01_LOG_?>
				else if ((strstr(&sprintf_buf[6], "LOG_")) != NULL && sprintf_buf[11] == '>')
				{
					char log_buf[100];
					if(sprintf_buf[10] == '?')
					{
						LOG_DATA log_data;
						if(EPROM.LOG_NUM == 0)
						{
							sprintf_len = sprintf((char *)sprintf_buf, "<NO_LOG>\r\n");
							return sprintf_len;
						}
						if(EPROM.LOG_NUM > 5)
						{
							for(i=EPROM.LOG_NUM;i>=EPROM.LOG_NUM-4;i--)
							{
								EEPROM_Read_Str(LOG_ADDR+i*40, (uint8 *)&log_data, sizeof(struct LOG_DATA));
								sprintf(log_buf, "<%03u> %s P:%01u, R1:%s, R2:%s\r\n", i, log_data.time, log_data.pwr_stat, (char *)log_data.R1_Power, (char *)log_data.R2_Power);
								UART0Write_Str((uint8 *)log_buf); 
							}
							return 0;						  
						}
						else
						{
							for(i=EPROM.LOG_NUM;i>0;i--)
							{
								EEPROM_Read_Str(LOG_ADDR+i*40, (uint8 *)&log_data, sizeof(struct LOG_DATA));
								sprintf(log_buf, "<%03u> %s P:%01u, R1:%s, R2:%s\r\n", i, log_data.time, log_data.pwr_stat, (char *)log_data.R1_Power, (char *)log_data.R2_Power);
								UART0Write_Str((uint8 *)log_buf); 
							}
							return 0;			
						}
					}
					else if(sprintf_buf[10] == 'C')
					{
					  for(i=0;i<40;i++)  log_buf[i] = 0;
                        for(i=0;i<=50;i++)
						{
							EEPROM_Write((LOG_ADDR+i*40)%64, (LOG_ADDR+i*40)/64, (uint8*)&log_buf, MODE_8_BIT, sizeof(struct LOG_DATA));
							OSTimeDly(1);
						}
						EPROM.LOG_NUM = 0;
						Save_To_EPROM((uint8 *)&EPROM.LOG_NUM, 1);
						sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_LOG_C_OK>\r\n", EPROM.address);
					}
				}
				
				//<BP01_A_?> 查询当前光功率和波长 <BP01_1-15.00_1-20.00>
				else if ((strstr(&sprintf_buf[6], "A_?>")) != NULL)
				{
					uint8 power_temp1[7], power_temp2[7];

					if (power[0] >= 0)
					{
							power_temp1[0] = '+';
							data_temp16 =  power[0] * 100;
					}
					else
					{
							power_temp1[0] = '-';
							data_temp16 = (0 - power[0]) * 100;
					}
					power_temp1[1] = data_temp16 / 1000 + '0';
					power_temp1[2] = data_temp16 % 1000 / 100 + '0';
					power_temp1[3] = '.';
					power_temp1[4] = data_temp16 % 100 / 10 + '0';
					power_temp1[5] = data_temp16 % 10 + '0';
					power_temp1[6] = '\0';
					
					if (power[1] >= 0)
					{
							power_temp2[0] = '+';
							data_temp16 =  power[1] * 100;
					}
					else
					{
							power_temp2[0] = '-';
							data_temp16 = (0 - power[1]) * 100;
					}
					power_temp2[1] = data_temp16 / 1000 + '0';
					power_temp2[2] = data_temp16 % 1000 / 100 + '0';
					power_temp2[3] = '.';
					power_temp2[4] = data_temp16 % 100 / 10 + '0';
					power_temp2[5] = data_temp16 % 10 + '0';
					power_temp2[6] = '\0';
					
					sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_%01u%s_%01u%s>\r\n" , EPROM.address, EPROM.wavelength[0], power_temp1, EPROM.wavelength[1], power_temp2);							  
				}
				
				//<BP01_B_?>
				else if( strstr((char*)&sprintf_buf[6],"B_?>")!=NULL )
				{
					uint16_t EPROM_q_power[CHANNEL_NUM*2];
					for(i=0;i<CHANNEL_NUM*2;i++)
					{
//						if(EPROM.q_power[i] >= 0)
//						{
//							data_temp[0] = '+';
//							data_temp[7] = '+';
//							EPROM_q_power[i] =  EPROM.q_power[i]*100;
//						}
//						else
//						{
							data_temp[0] = '-';
							data_temp[7] = '-';
							EPROM_q_power[i] = (0 - EPROM.q_power[i])*100;
//						}
					}
					
					data_temp[1] = EPROM_q_power[0]/1000+'0';
					data_temp[2] = EPROM_q_power[0]%1000/100+'0';
					data_temp[3] = '.';
					data_temp[4] = EPROM_q_power[0]%100/10+'0';
					data_temp[5] = EPROM_q_power[0]%10+'0';
					data_temp[6] = '_';
					data_temp[8] = EPROM_q_power[1]/1000+'0';
					data_temp[9] = EPROM_q_power[1]%1000/100+'0';
					data_temp[10] = '.';
					data_temp[11] = EPROM_q_power[1]%100/10+'0';
					data_temp[12] = EPROM_q_power[1]%10+'0';
					data_temp[13] = '\0';
					
					sprintf_len = sprintf((char *)sprintf_buf,"<BP%02u_%s>\r\n", EPROM.address, data_temp);
				}
				
				//<BP01_POWER_?>
				else if( strstr((char*)&sprintf_buf[6],"POWER_?>")!=NULL )
				{
					if(GET_POWER_STAT)
						sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_POWER_0>\r\n", EPROM.address);
					else
						sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_POWER_1>\r\n", EPROM.address);
				}
				
				//<BP01_GUID_?>
				else if( strstr((char*)&sprintf_buf[6],"GUID_?>")!=NULL)
				{
				    if(sprintf_buf[11] == '?')
                        sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_GUID_%s>\r\n", EPROM.address, EPROM.GUID);
				}
				
				//<BP01_XX_JUST_-0.0>校准功率
				//<BP01_XX_JUST_-0.0>校准功率
				else if( (strstr((char*)&sprintf_buf[8],"_JUST_"))!=NULL && sprintf_buf[18]=='>' )
				{
						uint8_t adc_just, data_temp_ch;
						data_temp_ch = (sprintf_buf[6]-'0')*10 + (sprintf_buf[7]-'0');
						if(data_temp_ch >= 1 && data_temp_ch <= CHANNEL_NUM*2)
						{
								adc_just = (sprintf_buf[15]-'0')*10 + (sprintf_buf[17]-'0');
								/*if(power_count[data_temp_ch-1] >= -41.0)
								{
										if(sprintf_buf[14] == '+')
										{
												EPROM.fuhao_one[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=1;
												Save_To_EPROM((uint8_t *)&EPROM.fuhao_one, 8);
										}
										else if(sprintf_buf[14] == '-')
										{
												EPROM.fuhao_one[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=0;
												Save_To_EPROM((uint8_t *)&EPROM.fuhao_one, 8);
										}
										else goto send_err;

										EPROM.ADC_just_one[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]] = adc_just;
										Save_To_EPROM((uint8_t *)&EPROM.ADC_just_one, 8); 
								}
								else if(power_count[data_temp_ch-1] < -41.0)
								{
										if(sprintf_buf[14] == '+')
										{
												EPROM.fuhao_two[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=1;
												Save_To_EPROM((uint8_t *)&EPROM.fuhao_two, 8);
										}
										else if(sprintf_buf[14] == '-')
										{
												EPROM.fuhao_two[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=0;
												Save_To_EPROM((uint8_t *)&EPROM.fuhao_two, 8);
										}
										else goto send_err;
								 
										EPROM.ADC_just_two[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]] = adc_just;
										Save_To_EPROM((uint8_t *)&EPROM.ADC_just_two, 8); 
								}*/		
								
								
								if(sprintf_buf[14] == '+')
								{
										EPROM.fuhao[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=1;
										Save_To_EPROM((uint8_t *)&EPROM.fuhao[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]], 1);
								}
								else if(sprintf_buf[14] == '-')
								{
										EPROM.fuhao[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]]=0;
										Save_To_EPROM((uint8_t *)&EPROM.fuhao[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]], 1);
								}
								else goto send_err;

								EPROM.ADC_just[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]] = adc_just;
								Save_To_EPROM((uint8_t *)&EPROM.ADC_just[data_temp_ch-1][EPROM.wavelength[data_temp_ch-1]],1); 
								
								sprintf_len = sprintf((char *)sprintf_buf, "<BP%02u_%02u_JUST_OK>", EPROM.address, data_temp_ch );
						}
						else goto send_err;
				}
          //<BP01_SERVER_IP_xxx.xxx.xxx.xxx:xxxx>
				else if( (strstr((char*)&sprintf_buf[6],"SERVER_IP"))!=NULL)
				{
					uint8_t i,data_t;
					char *cp;
					uint8_t temp[17];
					unsigned long ip = 0;
					unsigned long port = 0;
					if(sprintf_buf[16] == '?')
					{
						ip = EPROM.serverIP;
						my_inet_ntoa((char *)temp,ip);
						sprintf_len = sprintf((char *)sprintf_buf,"<BP01_SERVER_IP_%s:%d>",(char *)temp,(int)EPROM.serverPORT);
					}else
					{
						cp = strchr((char *)&sprintf_buf[16],':');
						if(cp != NULL)
						{
							cp++;
							for(i = 0 ; i < 5 ; i ++)
						    {
								data_t = *(cp+i);
						        if((data_t >= '0' && data_t <='9'))
								{
									port = port*10 + (data_t-'0');
								}else if(data_t == '>')
								{
									i = 6;
								}else goto send_err;
						    }
						}
						EPROM.serverPORT = port;
						if(my_inet_aton((char *)&sprintf_buf[16],&ip))
						{
							
							EPROM.serverIP = ip;
							Save_To_EPROM((uint8_t *)&EPROM.serverPORT,8);
							my_inet_ntoa((char *)temp,ip);
							sprintf_len = sprintf((char *)sprintf_buf,"<BP01_SERVER_IP_%s:%d_OK\r\n",(char *)temp,(int)EPROM.serverPORT);
							sprintf((char *)GSM_BUF9,"AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r\n",(char *)temp,(int)EPROM.serverPORT);
							sprintf_len = sprintf((char *)sprintf_buf,"%s",(char *)GSM_BUF9);
						}else goto send_err;
					}
				}
				 //<BP01_CIRCLE_TIME_xxxx>
				else if((strstr((char*)&sprintf_buf[6],"CIRCLE_TIME"))!=NULL && (sprintf_buf[19] == '>' || sprintf_buf[22] == '>') )
				{
					uint8_t data_t,i;
					unsigned long time_temp = 0;
					data_t = sprintf_buf[18];
					if(data_t == '?')
					{
						sprintf_len = sprintf((char *)sprintf_buf,"<BP01_CIRCLE_TIME_%d>",(int)EPROM.circle_time);
					}else
					{
						for(i = 0 ; i < 4 ;i++)
						{
							data_t = sprintf_buf[18+i];
							if(data_t >= '0' && data_t <='9')
							{
								time_temp = time_temp *10 + (data_t - '0');
							}else goto send_err;
						}
						EPROM.circle_time = time_temp;
						Save_To_EPROM((uint8_t *)&EPROM.circle_time,4);
						sprintf_len = sprintf((char *)sprintf_buf,"<BP01_CIRCLE_TIME_OK>");
					}
					
				}
				 //<BP01_USE_TCP_x>
				else if((strstr((char*)&sprintf_buf[6],"USE_TCP"))!=NULL && sprintf_buf[15] == '>'  )
				{
					uint8_t data_t,i;
					unsigned long time_temp = 0;
					data_t = sprintf_buf[14];
					if(data_t == '?')
					{
						data_t = EPROM.use_TCP +'0';
						sprintf_len = sprintf((char *)sprintf_buf,"<BP01_USE_TCP_%c>",data_t);
					}else if(data_t == '0' || data_t == '1')
					{
						
						EPROM.use_TCP = data_t - '0';
						Save_To_EPROM((uint8_t *)&EPROM.use_TCP,1);
						sprintf_len = sprintf((char *)sprintf_buf,"<BP01_USE_TCP_OK>");
					}
					
				}
				
				else
				{
					send_err:
						sprintf_len = sprintf((char *)sprintf_buf, "<CMD_ERR>\r\n");
					  if(GSM_CMD_FLAG)
						  sim800c_send(GSM_MSG_ERR1, 16);
						return sprintf_len;
				}
           }
    }
	//<RESUME>  恢复出厂设置
	else if( strstr((char*)&sprintf_buf[1], "RESUME") !=NULL )
	{
		UART0Write_Str((uint8 *)"<RESUME_OK>\r\n");
		delay_nms(100);
		GSM_BUSY = 1;
      restore_set();
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
		Reset_Handler();        //实行软件复位 
	}	
	
    return sprintf_len;
}




