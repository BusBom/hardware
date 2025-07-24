#include "bluetooth.h"
#include "usart.h"
#include <string.h>
#include <stdbool.h>

// 버퍼 및 인덱스 정의
static char rx_buffer[RX_BUFFER_SIZE];
static char bt_received_line[RX_BUFFER_SIZE];
static uint8_t rx_data;               // 한 바이트 수신 임시 저장
static uint32_t rx_index = 0;         // 현재 버퍼 인덱스

bool msg_from_bluetooth = false;

// UART 초기화 (인터럽트 수신 시작)
void bluetooth_init() {
	rx_index = 0;
	memset(rx_buffer, 0, RX_BUFFER_SIZE);
	HAL_UART_Receive_IT(&huart1, &rx_data, 1);  // 1바이트 수신 시작
}

// 버퍼 초기화 함수
void rx_buffer_clear() {
	rx_index = 0;
	memset(rx_buffer, 0, RX_BUFFER_SIZE);
}

// 외부에서 버퍼 내용을 읽을 수 있도록 복사
void read_from_bluetooth(char *buf, int size) {
	memcpy(buf, rx_buffer, size);
}

void get_line_from_bluetooth(char* buf){
	memcpy(buf, bt_received_line, strlen(bt_received_line));
	memset(bt_received_line, '\0', sizeof(bt_received_line));
	msg_from_bluetooth = false;
}

// UART 수신 완료 콜백 (1바이트마다 호출)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		if (rx_index < RX_BUFFER_SIZE - 1) {
			rx_buffer[rx_index++] = rx_data;

			// 종료 문자가 수신되면 전체 메시지를 전송
			if (rx_data == '\n') {
				msg_from_bluetooth = true;
				read_from_bluetooth(bt_received_line, rx_index); //flag 등 활용해서 받기
				rx_buffer_clear();  // 메시지 처리 후 버퍼 초기화
			}
		} else {
			// 버퍼 오버플로우 방지
			rx_buffer_clear();
			msg_from_bluetooth = false;
		}

		// 다음 바이트 수신 재시작
		HAL_UART_Receive_IT(&huart1, &rx_data, 1);
	}
}

// UART 에러 발생 시 콜백 (인터럽트 재설정 포함)
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		rx_buffer_clear();
		msg_from_bluetooth = false;
		// 인터럽트 기반 수신 재설정
		HAL_UART_Receive_IT(&huart1, &rx_data, 1);
	}
}
