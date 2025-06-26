/*
 * bluetooth.h
 *
 *  Created on: Jun 24, 2025
 *      Author: 2-10
 */

#ifndef INC_BLUETOOTH_H_
#define INC_BLUETOOTH_H_

#define RX_BUFFER_SIZE 100

void bluetooth_init();
void rx_buffer_clear();

void get_line_from_bluetooth(char* buf);
void read_from_bluetooth(char* buf, int size);

#endif /* INC_BLUETOOTH_H_ */
