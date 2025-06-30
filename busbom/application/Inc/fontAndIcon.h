/*
 * fontAndIcon.h
 *
 *  Created on: Jun FONT_WIDTH0, 2025
 *      Author: 2-10
 */

#ifndef INC_FONTANDICON_H_
#define INC_FONTANDICON_H_

#include <stdint.h>

#include <stdint.h>

// FONT_WIDTHxFONT_HEIGHT font for characters '0'-'9', 'A'-'Z'
// Each character is stored as uint8_t[FONT_HEIGHT][FONT_WIDTH]

#define FONT_HEIGHT 7
#define FONT_WIDTH 3

void draw_frame(uint8_t r, uint8_t g, uint8_t b);
void write_character(int y, int x, int r, int g, int b, char c);

#endif /* INC_FONTANDICON_H_ */
