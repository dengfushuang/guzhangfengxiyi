#ifndef __CMD_PROCESS_H_
#define __CMD_PROCESS_H_


extern uint8 LOG_Stade[][4];
extern uint8 LOG_Mode [][7];

extern uint16 charsrt_data(char *x, uint8 len ,uint8 *err);
extern uint16 Cmd_process( char *sprintf_buf );

#endif //__CMD_PROCESS_H_

