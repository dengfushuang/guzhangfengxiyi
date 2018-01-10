#include "unicode.h"
#include "string.h"
#define CODE_LEN 58
// ��    ��    ��    ��  ָ    ��   ��   ʽ   ʧ   ��   ��   ��
// 6210 529f  5bc6 7801 6307 4ee4 683c 5f0f 5931 8d25 9519 8bef
MY_UNICODE UnicodeArry[CODE_LEN]={"��","7535","Դ","6E90","��","6B63","��","5E38",
                                  "��","8F6F","��","4EF6","��","7248","��","672C",
                                  "��","544A","��","8B66","��","529F","��","7387",
	                              "��","5DE5","��","4F5C","��","6CE2","��","957F",
                                  "��","5149","��","5F53","ǰ","524D","Ŀ","76EE",
						          "��","6807","��","53F7","��","7801","��","6210",
						          "��","529F","��","5BC6","ָ","6307","��","4EE4",
						          "��","683C","ʽ","5F0F","ʧ","5931","��","8D25",
								  "��","9519","��","8BEF","��","7801","��","8BBE",
								  "��","7F6E","��","5F02",
                                  "0","0030","1","0031","2","0032","3","0033","4","0034",
                                  "5","0035","6","0036","7","0037","8","0038","9","0039",
								  "R","0052","V","0056","m","006D","n","006E","B","0042",
								  ",","FF0C",":","FF1A",".","002E",{"\r\n","000D"},{"\n\0","000A"}
							      };


void clear_Arry(char *src,unsigned int len)
{
	unsigned int i,j;
	for(i = 0;i < len;i++)
	{
		*src = '\0';
		src++;
	}
}
static unsigned int StringToUnicode_1(const char * src,char *dec)
{
	unsigned int src_len,i,j,flag,t_len=0;
	src_len = strlen(src);
	if(src_len > 0)
	{
		for(i = 0 ; i < src_len ;)
		{
			/*if((src[i] >= '0') && (src[i] <= '9'))
			{
				flag = 0;
			}else if((src[i] >= 'A') && (src[i] <= 'Z'))
			{
				flag = 0;
			}else if((src[i] >= 'a') && (src[i] <= 'z'))
			{
				flag = 0;
			}
			else if((src[i] == '\r')||(src[i] == '\n'))
			{
				flag = 0;
			}*/
			if((src[i] >= 0x00) && (src[i] <= 0x7E) )
			{
				flag = 0;
			}
			else
			{
				flag = 1;
			}
			if(flag == 0)
			{
				for(j = 0 ;j < CODE_LEN ; j++)
				{
					if(src[i] == UnicodeArry[j].index[0])
					{
						dec[t_len*4] = UnicodeArry[j].code[0];
						dec[t_len*4+1] = UnicodeArry[j].code[1];
						dec[t_len*4+2] = UnicodeArry[j].code[2];
						dec[t_len*4+3] = UnicodeArry[j].code[3];
					}
				}
                t_len++;
				i++;
			}else
            {
				for(j = 0 ;j < CODE_LEN ; j++)
				{
					if((src[i] == UnicodeArry[j].index[0])&&(src[i+1] == UnicodeArry[j].index[1]))
					{
						dec[t_len*4] = UnicodeArry[j].code[0];
						dec[t_len*4+1] = UnicodeArry[j].code[1];
						dec[t_len*4+2] = UnicodeArry[j].code[2];
						dec[t_len*4+3] = UnicodeArry[j].code[3];
					}
				}
				t_len++;
				i+=2;
			}				
		}
		dec[t_len*4] = '\0';		
	}
	else
	{
		t_len = 0;
	}
	return t_len*4;
}
unsigned int  StringToUnicode(const char * src,char *dec)
{
	return  StringToUnicode_1(src,dec);
}
////����汾V1.00
////8F6F 4EF6 7248 672C 0056 0031 002E 0030 0030 000D 000A

////��Դ����
////7535 6E90 6B63 5E38 000D 000A

////R1�澯����XXX.XXdBm
////0052 0031 544A 8B66 529F 7387 0058 0058 0058 002E 0058 0058 0064 0042 006D 000D 000A

////R1�澯����XXX.XXdBm
////0052 0031 544A 8B66 529F 7387 0058 0058 0058 002E 0058 0058 0064 0042 006D 000D 000A

////R1��������1310nm
////0052 0031 5DE5 4F5C 6CE2 957F 0031 0033 0031 0030 006E 006D 000D 000A

////R1��������1310nm
////0052 0031 5DE5 4F5C 6CE2 957F 0031 0033 0031 0030 006E 006D 000D 000A

////R1��������ǰ�⹦��XXX.XXdBm
////0052 0031 6B63 5E38 FF0C 5F53 524D 5149 529F 7387 0058 0058 0058 002E 0058 0058 0064 0042 006D 000D 000A

////R1��������ǰ�⹦��XXX.XXdBm
////0052 0031 6B63 5E38 FF0C 5F53 524D 5149 529F 7387 0058 0058 0058 002E 0058 0058 0064 0042 006D 000D 000A

////Ŀ�����1��136XXXXXXXX
////76EE 6807 53F7 7801 0031 FF1A 0031 0033 0036 0058 0058 0058 0058 0058 0058 0058 0058 000D 000A