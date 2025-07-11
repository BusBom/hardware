#ifndef __MP3_CONTROL_H__
#define __MP3_CONTROL_H__

#include <stdint.h>

/*
 * 123456789십백천(번)(버스가 도착했습니다.)
 * */

void mp3_init();
void mp3_play_track(uint8_t track);
void mp3_pause(void);
void mp3_resume(void);
void mp3_stop(void);
void mp3_set_volume(uint8_t volume);
void mp3_send_cmd(uint8_t cmd, uint8_t param1, uint8_t param2);
void mp3_bus_inform(char (*arr)[100], uint32_t size);
void call_number(char *number, int* number_index);
#endif
