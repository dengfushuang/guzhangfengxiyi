#ifndef _UNICODE_H
#define _UNICODE_H
typedef struct A{
    char index[2];
    char code[4];
}MY_UNICODE;
unsigned int StringToUnicode(const char * src,char *dec);
void clear_Arry(char *src,unsigned int len);
#endif
