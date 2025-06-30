#include "fontAndIcon.h"
#include "RGBmatrix.h"

#include <ctype.h>

static const uint8_t font_0[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_1[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 0 },
    { 1, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 1, 1, 1 },
};

static const uint8_t font_2[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
};

static const uint8_t font_3[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_4[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
};

static const uint8_t font_5[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_6[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_7[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
};

static const uint8_t font_8[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_9[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_A[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_B[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
};

static const uint8_t font_C[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 0, 1, 1 },
};

static const uint8_t font_D[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
};

static const uint8_t font_E[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
};

static const uint8_t font_F[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
};

static const uint8_t font_G[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 0, 1, 1 },
};

static const uint8_t font_H[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_I[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 1, 1, 1 },
};

static const uint8_t font_J[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_K[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_L[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
};

static const uint8_t font_M[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_N[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_O[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 0, 1, 0 },
};

static const uint8_t font_P[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
};

static const uint8_t font_Q[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 1, 0 },
    { 0, 0, 1 },
};

static const uint8_t font_R[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_S[FONT_HEIGHT][FONT_WIDTH] = {
    { 0, 1, 1 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 1, 1, 0 },
};

static const uint8_t font_T[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
};

static const uint8_t font_U[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
};

static const uint8_t font_V[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
};

static const uint8_t font_W[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_X[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};

static const uint8_t font_Y[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
};

static const uint8_t font_Z[FONT_HEIGHT][FONT_WIDTH] = {
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 1, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 1 },
};

/*
 * 64x32 -> 58x26 -> 29x13
 * */

void draw_frame(uint8_t r, uint8_t g, uint8_t b) {
    // 중앙 수직선 (x = 31, 32)
    for (uint8_t y = 0; y < HUB75_HEIGHT; y++) {
        HUB75_SetPixel(31, y, r, g, b);
        HUB75_SetPixel(32, y, r, g, b);
    }

    // 중앙 수평선 (y = 15, 16)
    for (uint8_t x = 0; x < HUB75_WIDTH; x++) {
        HUB75_SetPixel(x, 15, r, g, b);
        HUB75_SetPixel(x, 16, r, g, b);
    }

    // 상단 테두리 (y = 0, 1)
    for (uint8_t x = 0; x < HUB75_WIDTH; x++) {
        HUB75_SetPixel(x, 0, r, g, b);
        HUB75_SetPixel(x, 1, r, g, b);
    }

    // 하단 테두리 (y = 30, 31)
    for (uint8_t x = 0; x < HUB75_WIDTH; x++) {
        HUB75_SetPixel(x, HUB75_HEIGHT - 2, r, g, b);
        HUB75_SetPixel(x, HUB75_HEIGHT - 1, r, g, b);
    }

    // 좌측 테두리 (x = 0, 1)
    for (uint8_t y = 0; y < HUB75_HEIGHT; y++) {
        HUB75_SetPixel(0, y, r, g, b);
        HUB75_SetPixel(1, y, r, g, b);
    }

    // 우측 테두리 (x = 62, 63)
    for (uint8_t y = 0; y < HUB75_HEIGHT; y++) {
        HUB75_SetPixel(HUB75_WIDTH - 2, y, r, g, b);
        HUB75_SetPixel(HUB75_WIDTH - 1, y, r, g, b);
    }
}

void write_character(int y, int x, int r, int g, int b, char c) {
    static const uint8_t (*bitmap)[FONT_WIDTH] = NULL;

    if (c >= '0' && c <= '9') {
        switch (c) {
            case '0': bitmap = font_0; break;
            case '1': bitmap = font_1;break;
            case '2': bitmap = font_2;break;
            case '3': bitmap = font_3;break;
            case '4': bitmap = font_4;break;
            case '5': bitmap = font_5;break;
            case '6': bitmap = font_6;break;
            case '7': bitmap = font_7;break;
            case '8': bitmap = font_8;break;
            case '9': bitmap = font_9;break;
        }
    } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        switch (toupper((unsigned char)c)) {
            case 'A': bitmap = font_A;break;
            case 'B': bitmap = font_B;break;
            case 'C': bitmap = font_C;break;
            case 'D': bitmap = font_D;break;
            case 'E': bitmap = font_E;break;
            case 'F': bitmap = font_F;break;
            case 'G': bitmap = font_G;break;
            case 'H': bitmap = font_H;break;
            case 'I': bitmap = font_I;break;
            case 'J': bitmap = font_J;break;
            case 'K': bitmap = font_K;break;
            case 'L': bitmap = font_L;break;
            case 'M': bitmap = font_M;break;
            case 'N': bitmap = font_N;break;
            case 'O': bitmap = font_O;break;
            case 'P': bitmap = font_P;break;
            case 'Q': bitmap = font_Q;break;
            case 'R': bitmap = font_R;break;
            case 'S': bitmap = font_S;break;
            case 'T': bitmap = font_T;break;
            case 'U': bitmap = font_U;break;
            case 'V': bitmap = font_V;break;
            case 'W': bitmap = font_W;break;
            case 'X': bitmap = font_X;break;
            case 'Y': bitmap = font_Y;break;
            case 'Z': bitmap = font_Z;break;
        }
    }

    if (bitmap) {
        for (int row = 0; row < FONT_HEIGHT; row++) {
            for (int col = 0; col < FONT_WIDTH; col++) {
                if (bitmap[row][col]) {
                    HUB75_SetPixel(x + col, y + row, r, g, b);
                }
            }
        }
    }
}

void write(int y, int x, int r, int g, int b, char* str, int size){

}


