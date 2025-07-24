/*
 * matrix.h
 *
 *  Created on: Jun 26, 2025
 *      Author: shipg
 */

/*
 * PA2 : R1
 * PC4 : G1
 * PB5 : B1
 * PB4 : R2
 * PB10 : G2
 * PA8 : B2
 * PA3 : A
 * PC7 : B
 * PB6 : C
 * PA7 : D
 * PA6 : CLK
 * PA5 : LAT
 * PB9 : OE
 * */

/*
 * hub75.h
 *
 *  Created on: Jun 26, 2025
 *      Author: shipg (Modified for standard HUB75)
 */

#ifndef INC_RGBMATRIX_H_
#define INC_RGBMATRIX_H_

#include "gpio.h"

#define HUB75_WIDTH    64
#define HUB75_HEIGHT   32
#define HUB75_COLOR_DEPTH 8  // 8-bit per color channel


typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} HUB75_Pin;

typedef struct {
    // Color lines for upper and lower panels
    HUB75_Pin R1, G1, B1;
    HUB75_Pin R2, G2, B2;

    // Row address lines (A ~ D only for 1/8 scan)
    HUB75_Pin A, B, C, D;

    // Control lines
    HUB75_Pin CLK;   // Clock
    HUB75_Pin LAT;   // Latch
    HUB75_Pin OE;    // Output Enable
} HUB75_Config;


// 초기화 및 제어 함수
void HUB75_Init(const HUB75_Config* config);
void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void HUB75_FillScreen(uint8_t r, uint8_t g, uint8_t b);
void HUB75_UpdateScreen(void);
void HUB75_DisplayOn(void);
void HUB75_DisplayOff(void);
void HUB75_UpdateRow_ISR(void);

#endif /* INC_HUB75_H_ */

