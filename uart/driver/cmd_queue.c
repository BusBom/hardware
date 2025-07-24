#include "cmd_queue.h"

void init(cmd_queue_t* queue){
    queue->frnot = queue->rear = -1;
}