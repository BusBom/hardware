#include "mp3.h"
#include "main.h"

#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart6;

/**Sound Track
 * Track : Content
 * 1 ~ 9 : 1 ~ 9
 * 10 	 : 10
 * 11	 : 100
 * 12	 : 1000
 * 13	 : 번
 * 14	 : 잠시 후 도착버스는
 * 15	 : 입니다
 * */

#define SOUND_TRCK_10 10
#define SOUND_TRCK_100 11
#define SOUND_TRCK_1000 12
#define SOUND_TRCK_BUN 13
#define SOUND_TRCK_SOON 14
#define SOUND_TRCK_IPNIDA 15

// 공통 명령 전송
void mp3_send_cmd(uint8_t cmd, uint8_t param1, uint8_t param2) {
	uint8_t packet[10] = { 0x7E, 0xFF, 0x06, cmd, 0x00, param1, param2, 0x00,
			0x00, 0xEF };

	uint16_t sum = 0;
	for (int i = 1; i < 7; i++) {
		sum += packet[i];
	}
	sum = 0xFFFF - sum + 1;
	packet[7] = (sum >> 8) & 0xFF;
	packet[8] = sum & 0xFF;

	HAL_UART_Transmit(&huart6, packet, 10, 10);
}

void mp3_init() {
	HAL_Delay(3000);
	mp3_send_cmd(0x3F, 0x00, 0x00);
	HAL_Delay(1000);
	mp3_set_volume(0);
	HAL_Delay(1000);
	for (int i = 1; i <= 15; i++) {
		mp3_play_track(i);
		HAL_Delay(600);
	}
	mp3_set_volume(30);
	HAL_Delay(1000);
}

// ▶️ 트랙 재생
void mp3_play_track(uint8_t track) {
	mp3_send_cmd(0x12, 0x00, track);
	//printf("▶️ 트랙 %d 재생\r\n", track);
}

// ⏸️ 일시 정지
void mp3_pause(void) {
	mp3_send_cmd(0x0E, 0x00, 0x00);
	//printf("⏸️ 일시 정지\r\n");
}

// ▶️ 재개
void mp3_resume(void) {
	mp3_send_cmd(0x0D, 0x00, 0x00);
	//printf("▶️ 재개\r\n");
}

// ⏹️ 정지
void mp3_stop(void) {
	mp3_send_cmd(0x16, 0x00, 0x00);
	//printf("⏹️ 정지\r\n");
}

// 🔊 볼륨 설정 (0~30)
void mp3_set_volume(uint8_t volume) {
	if (volume > 30)
		volume = 30;
	mp3_send_cmd(0x06, 0x00, volume);
	//printf("🔊 볼륨: %d\r\n", volume);
}

void mp3_bus_inform(char (*arr)[100], uint32_t size) {
	mp3_play_track(SOUND_TRCK_SOON); //잠시 후 도착 버스는
	HAL_Delay(2000);

	for (int i = 0; i < size; i++) {

		if(arr[i][0] == ' '){
			continue;
		}

		char number[100] = { '\0', };
		int number_index = 0;

		for (int j = 0; arr[i][j] != '\0'; j++) {
			if (arr[i][j] >= '0' && arr[i][j] <= '9') {
				number[number_index++] = arr[i][j] - '0';
			} else {
				call_number(number, &number_index);

				mp3_play_track(arr[i][j]);
				HAL_Delay(600);
			}
		}

		call_number(number, &number_index);


		mp3_play_track(SOUND_TRCK_BUN);

		HAL_Delay(800);
	}

	mp3_play_track(SOUND_TRCK_IPNIDA); //버스입니다.
	HAL_Delay(1400);
}

void call_number(char *number, int *number_index) {
	if (*number_index > 0) {
		switch (*number_index) {
		case 1:
			mp3_play_track(number[0]);
			HAL_Delay(550);
			break;
		case 2:
			if (number[0] > 1) {
				mp3_play_track(number[0]);
				HAL_Delay(550);
			}
			mp3_play_track(SOUND_TRCK_10);
			HAL_Delay(550);
			if (number[1] > 0) {
				mp3_play_track(number[1]);
				HAL_Delay(550);
			}
			break;
		case 3:
			if (number[0] > 1) {
				mp3_play_track(number[0]);
				HAL_Delay(550);
			}
			mp3_play_track(SOUND_TRCK_100);
			HAL_Delay(600);
			if (number[1] > 0) {
				if (number[1] > 1) {
					mp3_play_track(number[1]);
					HAL_Delay(550);
				}
				mp3_play_track(SOUND_TRCK_10);
				HAL_Delay(550);
			}
			if (number[2] > 0) {
				mp3_play_track(number[2]);
				HAL_Delay(550);
			}
			break;
		case 4:
			if (number[0] > 1) {
				mp3_play_track(number[0]);
				HAL_Delay(550);
			}
			mp3_play_track(SOUND_TRCK_1000);
			HAL_Delay(550);
			if (number[1] > 0) {
				if (number[1] > 1) {
					mp3_play_track(number[1]);
					HAL_Delay(550);
				}
				mp3_play_track(SOUND_TRCK_100);
				HAL_Delay(600);
			}
			if (number[2] > 0) {
				if (number[2] > 1) {
					mp3_play_track(number[2]);
					HAL_Delay(550);
				}
				mp3_play_track(SOUND_TRCK_10);
				HAL_Delay(550);
			}
			if (number[3] > 0) {
				mp3_play_track(number[3]);
				HAL_Delay(550);
			}
			break;
		};

		*number_index = 0;
	}
}
