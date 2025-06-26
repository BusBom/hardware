/*
 * matrix.h
 *
 *  Created on: Jun 26, 2025
 *      Author: shipg
 */

/*
 * PA10 : R1
 * PB3 : G1
 * PB5 : B1
 * PB4 : R2
 * PB10 : G2
 * PA8 : B2
 * PA9 : A
 * PC7 : B
 * PB6 : C
 * PA7 : D
 * PA6 : E
 * PA5 : CLK
 * GND
 *
 * PB0 : LAT
 * PB8 : OE
 * */

#ifndef INC_RGBMATRIX_H_
#define INC_RGBMATRIX_H_

#include "gpio.h"

#define HUB75_WIDTH 64
#define HUB75_HEIGHT 32

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} HUB75_Pin;

typedef struct {
    HUB75_Pin R1;
    HUB75_Pin G1;
    HUB75_Pin B1;
    HUB75_Pin R2;
    HUB75_Pin G2;
    HUB75_Pin B2;
    HUB75_Pin A;
    HUB75_Pin B;
    HUB75_Pin C;
    HUB75_Pin D;
    HUB75_Pin E;
    HUB75_Pin CLK;
    HUB75_Pin LAT;
    HUB75_Pin OE;
} HUB75_Config;

extern HUB75_Config hub75_pins;

void HUB75_Init(HUB75_Config config);
void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void HUB75_UpdateScreen(void);
void HUB75_FillScreen(uint8_t r, uint8_t g, uint8_t b);
void HUB75_DisplayOn(void);
void HUB75_DisplayOff(void);

#endif /* INC_RGBMATRIX_H_ */
