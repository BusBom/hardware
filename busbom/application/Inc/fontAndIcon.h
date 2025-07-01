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
/*
 * 60*28 -> 30*14 -> 28*12 -> 24*12
 * */

#define FONT_HEIGHT 12
#define FONT_WIDTH 4
#define DISPLAY_SECTION_CNT  4
#define DISPLAY_STRING_LENGTH 100

void draw_frame(uint8_t r, uint8_t g, uint8_t b);
void draw_big_frame(uint8_t r, uint8_t g, uint8_t b);
void write_char_display(int y, int x, int r, int g, int b, char c);
void write_str_display(int y, int x, int r, int g, int b, char* str, int size);
void draw_jeongwangpan(int r, int g, int b, char (*str)[DISPLAY_STRING_LENGTH]);
void draw_clock(int r, int g, int b, char (*str)[DISPLAY_STRING_LENGTH]);
#endif /* INC_FONTANDICON_H_ */
