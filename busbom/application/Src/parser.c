/*
 * parser.c
 *
 *  Created on: Jun 26, 2025
 *      Author: 2-10
 */

#include "parser.h"

#include <string.h>

int parse(char* src, char (*dest)[100], char delimeter, int max_tokens) {
    char delim[2] = {delimeter, '\0'};  // strtok은 문자열을 기대하므로 char 배열로 만듦
    char* result;
    int index = 0;

    result = strtok(src, delim);

    while (result != NULL && index < max_tokens) {
        strncpy(dest[index], result, 99);
        dest[index][99] = '\0';  // null-terminate to avoid overflow
        index++;
        result = strtok(NULL, delim);
    }

    return index;  // 얼마나 잘렸는지 반환 (선택)
}
