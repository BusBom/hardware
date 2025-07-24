#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H

#define CONN 0
#define BUS 1
#define CLOCK 2
#define ONOFF 3

#define SIZE 100

typedef char element;

typedef struct 
{
    element data[SIZE];
    int rear, frnot;
} cmd_queue_t;

void init(cmd_queue_t* queue);

#endif