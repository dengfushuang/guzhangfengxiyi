#include "my_inet.h"
#include "stdio.h"
#include "string.h"
int my_inet_aton (const char *cp,unsigned long *ipaddr)
{
    int dots = 0;  //2
    unsigned long addr = 0;
    unsigned long val= 0, base = 10;
	char c;
    do
    {
         c= *cp;
        switch (c)
        {
            case '0': 
            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                      val = (val * base) + (c - '0');
                      break;
            case '.': if (++dots > 3){   return 0;};
            case ':':if (val > 255){ return 0; }
                      addr = addr << 8 | val;
                      val = 0;
                      break;
            default: return 0;
        }
		if(c == ':')
		{
			break;
		}
		cp++;
    } while (dots <= 3) ;
    if (dots == 3)
    {
		*ipaddr = addr;
	}
  return 1;
}
char * my_inet_ntoa(char *p,unsigned  int ipadr)
{
    unsigned int ip;
    ip = ipadr;
    sprintf(p,"%d.%d.%d.%d",((ip&0xFF000000)>>24),((ip&0x00FF0000)>>16),((ip&0x0000FF00)>>8),((ip&0x000000FF)));
	return p;
}
/*float my_atof(char *str, int *err){
    float ret = 0.0;
    int dot, dpos = 1;
    char *cp;

    if(((str[0] != '-') && (str[0] != '+')) && ((str[0] < '0') && (str[0] > '9')))
    {
		*err = -1; 
        return 0;
        
    }
        
//    if(strlen(str) > 10)
//    {
//		*err = -1; 
//        return 0; 
//    }
    if((str[0] == '-') || (str[0] == '+'))
        cp = &str[1];
    else
        cp = str;

    while(1){
        if(dot>1)
        {
            break;
           
        }
            
        if((*cp >= '0') && (*cp <= '9')){
            ret *= 10;
            ret += *cp - '0';
            if(dot)
                dpos *= 10;
        }
        else if(*cp == '.')
            dot++;
        else
            break;
        cp++;
    }

    if(dot)
        ret /= dpos;
    if(str[0] == '-')
        ret = 0 - ret;
    
    *err = 0;
    return ret;
}*/

float my_atof(char *str){
    float ret = 0.0;
    int dot, dpos = 1;
    char *cp;

    if(((str[0] != '-') && (str[0] != '+')) && ((str[0] < '0') && (str[0] > '9')))
        return 0;
        
    // if(strlen(str) > 10)
    //     return 0;

    if((str[0] == '-') || (str[0] == '+'))
        cp = &str[1];
    else
        cp = str;

    while(1){
        if(dot > 1)
            break;
        if((*cp >= '0') && (*cp <= '9')){
            ret *= 10.0;
            ret += *cp - '0';
            if(dot)
                dpos *= 10;
        }
        else if(*cp == '.')
            dot++;
        else
            break;
        cp++;
    }
	
    if(str[0] == '-')
        ret = 0 - ret;
    if(dot)
        ret /= (float)dpos;

    return ret;
}
















