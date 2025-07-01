/*
 * matrix.c
 *
 *  Created on: Jun 26, 2025
 *      Author: shipg
 */

/*
 * PA10 : R1
 * PC4 : G1
 * PB5 : B1
 * PB4 : R2
 * PB10 : G2
 * PA8 : B2
 * PA9 : A
 * PC7 : B
 * PB6 : C
 * PA7 : D
 * PA6 : CLK
 * PA5 : LAT
 * PB9 : OE
 * */

#include "RGBmatrix.h"
#include "main.h"


static HUB75_Config hub75_pins = {
		.R1 = { GPIOA, GPIO_PIN_10 },
		.G1 = { GPIOC,GPIO_PIN_4 },
		.B1 = { GPIOB, GPIO_PIN_5 },
		.R2 = { GPIOB, GPIO_PIN_4 },
		.G2 = {GPIOB, GPIO_PIN_10 },
		.B2 = { GPIOA, GPIO_PIN_8 },
		.A = { GPIOA, GPIO_PIN_9 },
		.B = { GPIOC, GPIO_PIN_7 },
		.C = { GPIOB, GPIO_PIN_6 },
		.D = { GPIOA, GPIO_PIN_7 },
		.CLK = { GPIOA, GPIO_PIN_6 },
		.LAT = { GPIOA, GPIO_PIN_5 },
		.OE = { GPIOB, GPIO_PIN_9 }
};

uint8_t rgb_framebuffer[HUB75_HEIGHT][HUB75_WIDTH][3];

static void pulse(GPIO_TypeDef *port, uint16_t pin) {
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
	__NOP();
	__NOP();
	__NOP();
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

void HUB75_Init(const HUB75_Config *config) {
	hub75_pins = *config;
}

void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
	if (x < HUB75_WIDTH && y < HUB75_HEIGHT) {
		rgb_framebuffer[y][x][0] = b ? 1 : 0;
		rgb_framebuffer[y][x][1] = r ? 1 : 0;
		rgb_framebuffer[y][x][2] = g ? 1 : 0;
	}
}

void HUB75_FillScreen(uint8_t r, uint8_t g, uint8_t b) {
	for (uint8_t y = 0; y < HUB75_HEIGHT; y++) {
		for (uint8_t x = 0; x < HUB75_WIDTH; x++) {
			HUB75_SetPixel(x, y, r, g, b);
		}
	}
}

void HUB75_DisplayOn(void) {
	HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_RESET);
}

void HUB75_DisplayOff(void) {
	HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_SET);
}

static void set_row_address(uint8_t row) {
	HAL_GPIO_WritePin(hub75_pins.A.port, hub75_pins.A.pin, (row >> 0) & 0x01);
	HAL_GPIO_WritePin(hub75_pins.B.port, hub75_pins.B.pin, (row >> 1) & 0x01);
	HAL_GPIO_WritePin(hub75_pins.C.port, hub75_pins.C.pin, (row >> 2) & 0x01);
	HAL_GPIO_WritePin(hub75_pins.D.port, hub75_pins.D.pin, (row >> 3) & 0x01);
	// E 핀 제거됨
}

void HUB75_UpdateScreen(void) {
	for (uint8_t row = 0; row < HUB75_HEIGHT / 2; row++) {
		set_row_address(row);
		HAL_GPIO_WritePin(hub75_pins.LAT.port, hub75_pins.LAT.pin,
				GPIO_PIN_RESET);
		HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_SET);

		for (uint8_t col = 0; col < HUB75_WIDTH; col++) {
			uint8_t r1 = rgb_framebuffer[row][col][0];
			uint8_t g1 = rgb_framebuffer[row][col][1];
			uint8_t b1 = rgb_framebuffer[row][col][2];
			uint8_t r2 = rgb_framebuffer[row + HUB75_HEIGHT / 2][col][0];
			uint8_t g2 = rgb_framebuffer[row + HUB75_HEIGHT / 2][col][1];
			uint8_t b2 = rgb_framebuffer[row + HUB75_HEIGHT / 2][col][2];

			HAL_GPIO_WritePin(hub75_pins.R1.port, hub75_pins.R1.pin, r1);
			HAL_GPIO_WritePin(hub75_pins.G1.port, hub75_pins.G1.pin, g1);
			HAL_GPIO_WritePin(hub75_pins.B1.port, hub75_pins.B1.pin, b1);
			HAL_GPIO_WritePin(hub75_pins.R2.port, hub75_pins.R2.pin, r2);
			HAL_GPIO_WritePin(hub75_pins.G2.port, hub75_pins.G2.pin, g2);
			HAL_GPIO_WritePin(hub75_pins.B2.port, hub75_pins.B2.pin, b2);

			pulse(hub75_pins.CLK.port, hub75_pins.CLK.pin);
		}

		pulse(hub75_pins.LAT.port, hub75_pins.LAT.pin);
		HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin,
				GPIO_PIN_RESET);

		HAL_Delay(1);  // 간단한 프레임 지연
	}
}

static uint8_t current_row = 0;
void HUB75_UpdateRow_ISR(void)
{
    HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_SET);  // 출력 OFF
    set_row_address(current_row);  // 행 주소 설정
    HAL_GPIO_WritePin(hub75_pins.LAT.port, hub75_pins.LAT.pin, GPIO_PIN_RESET);

    for (uint8_t col = 0; col < HUB75_WIDTH; col++) {
        uint8_t r1 = rgb_framebuffer[current_row][col][0];
        uint8_t g1 = rgb_framebuffer[current_row][col][1];
        uint8_t b1 = rgb_framebuffer[current_row][col][2];
        uint8_t r2 = rgb_framebuffer[current_row + HUB75_HEIGHT / 2][col][0];
        uint8_t g2 = rgb_framebuffer[current_row + HUB75_HEIGHT / 2][col][1];
        uint8_t b2 = rgb_framebuffer[current_row + HUB75_HEIGHT / 2][col][2];

        HAL_GPIO_WritePin(hub75_pins.R1.port, hub75_pins.R1.pin, r1);
        HAL_GPIO_WritePin(hub75_pins.G1.port, hub75_pins.G1.pin, g1);
        HAL_GPIO_WritePin(hub75_pins.B1.port, hub75_pins.B1.pin, b1);
        HAL_GPIO_WritePin(hub75_pins.R2.port, hub75_pins.R2.pin, r2);
        HAL_GPIO_WritePin(hub75_pins.G2.port, hub75_pins.G2.pin, g2);
        HAL_GPIO_WritePin(hub75_pins.B2.port, hub75_pins.B2.pin, b2);

        pulse(hub75_pins.CLK.port, hub75_pins.CLK.pin);
    }

    pulse(hub75_pins.LAT.port, hub75_pins.LAT.pin);       // 래치
    HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_RESET); // 출력 ON

    current_row = (current_row + 1) % (HUB75_HEIGHT / 2); // 다음 행
}

int onoff = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM10) {
    	if(onoff > 0)
    		HUB75_UpdateRow_ISR();
    }
}
