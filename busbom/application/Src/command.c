#include "command.h"

#include "parser.h"
#include "usart.h"
#include "bluetooth.h"
#include "RGBmatrix.h"
#include "fontAndIcon.h"
#include "mp3.h"

#include <string.h>

static char parsed_cmd_parameters[10][RX_BUFFER_SIZE];
char buses[DISPLAY_SECTION_CNT][DISPLAY_STRING_LENGTH];
char time[9][DISPLAY_STRING_LENGTH];

void command(char *data) {

	for (int i = 0; i < 10; i++) {
		memset(parsed_cmd_parameters[i], '\0', RX_BUFFER_SIZE);
	}

	char temp[RX_BUFFER_SIZE];

	strncpy(temp, data, RX_BUFFER_SIZE - 1);
	temp[RX_BUFFER_SIZE - 1] = '\0';  // 안정적 null 종료

	int count = parse(temp, parsed_cmd_parameters, ':', 10);

	if (strcmp(parsed_cmd_parameters[0], "ON") == 0) {
		extern int onoff;
		onoff = 1;
		draw_frame(0, 1, 1);
	} else if (strcmp(parsed_cmd_parameters[0], "OFF") == 0) {
		extern int onoff;
		onoff = 0;
		HUB75_DisplayOff();
	} else if (strcmp(parsed_cmd_parameters[0], "BUS") == 0) {
		memset(buses, 0, sizeof(buses));
		for (int i = 1; i < count - 1; i++) {
			strncpy(buses[i - 1], parsed_cmd_parameters[i], RX_BUFFER_SIZE - 1);
		}
		draw_jeongwangpan(1, 0, 0, buses);
		mp3_bus_inform(buses, count-2);
	} else if (strcmp(parsed_cmd_parameters[0], "TIME") == 0) {
		memset(time, 0, sizeof(time));
		for (int i = 1; i < count - 1; i++) {
			strncpy(time[i - 1], parsed_cmd_parameters[i], RX_BUFFER_SIZE - 1);
		}
		draw_clock(1, 0, 0, time);
	}
	else if(strcmp(parsed_cmd_parameters[0], "CONN") == 0){
		uint8_t conn[] = "1";
		HAL_UART_Transmit(&huart2, (uint8_t*)conn, sizeof(conn), 10);
	}
}
