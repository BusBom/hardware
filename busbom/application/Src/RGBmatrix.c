/*
 * matrix.c
 *
 *  Created on: Jun 26, 2025
 *      Author: shipg
 */


#include "RGBmatrix.h"


static uint8_t framebuffer[HUB75_HEIGHT][HUB75_WIDTH][3];
HUB75_Config hub75_pins;

static void pulse(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

void HUB75_Init(HUB75_Config config) {
    hub75_pins = config;
}

void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < HUB75_WIDTH && y < HUB75_HEIGHT) {
        framebuffer[y][x][0] = r ? 1 : 0;
        framebuffer[y][x][1] = g ? 1 : 0;
        framebuffer[y][x][2] = b ? 1 : 0;
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
    HAL_GPIO_WritePin(hub75_pins.E.port, hub75_pins.E.pin, (row >> 4) & 0x01);
}

void HUB75_UpdateScreen(void) {
    for (uint8_t row = 0; row < HUB75_HEIGHT / 2; row++) {
        set_row_address(row);
        HAL_GPIO_WritePin(hub75_pins.LAT.port, hub75_pins.LAT.pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_SET);

        for (uint8_t col = 0; col < HUB75_WIDTH; col++) {
            uint8_t r1 = framebuffer[row][col][0];
            uint8_t g1 = framebuffer[row][col][1];
            uint8_t b1 = framebuffer[row][col][2];
            uint8_t r2 = framebuffer[row + HUB75_HEIGHT / 2][col][0];
            uint8_t g2 = framebuffer[row + HUB75_HEIGHT / 2][col][1];
            uint8_t b2 = framebuffer[row + HUB75_HEIGHT / 2][col][2];

            HAL_GPIO_WritePin(hub75_pins.R1.port, hub75_pins.R1.pin, r1);
            HAL_GPIO_WritePin(hub75_pins.G1.port, hub75_pins.G1.pin, g1);
            HAL_GPIO_WritePin(hub75_pins.B1.port, hub75_pins.B1.pin, b1);
            HAL_GPIO_WritePin(hub75_pins.R2.port, hub75_pins.R2.pin, r2);
            HAL_GPIO_WritePin(hub75_pins.G2.port, hub75_pins.G2.pin, g2);
            HAL_GPIO_WritePin(hub75_pins.B2.port, hub75_pins.B2.pin, b2);

            pulse(hub75_pins.CLK.port, hub75_pins.CLK.pin);
        }

        pulse(hub75_pins.LAT.port, hub75_pins.LAT.pin);

        HAL_GPIO_WritePin(hub75_pins.OE.port, hub75_pins.OE.pin, GPIO_PIN_RESET);

        HAL_Delay(1);  // 간단한 딜레이
    }
}
