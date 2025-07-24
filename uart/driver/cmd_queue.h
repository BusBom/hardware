#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/kernel.h>

#define CMD_QUEUE_CAPACITY 64  // 최대 저장 가능 데이터 수 + 1 (구분을 위한 1칸 남김)

#define CONN 0
#define BUS 1
#define CLOCK 2
#define ONOFF 3

/**
 * @brief 커널용 원형 큐 구조체 정의
 */
typedef struct {
    uint8_t buffer[CMD_QUEUE_CAPACITY]; // 데이터 버퍼
    uint16_t head;                      // 데이터 삽입 위치
    uint16_t tail;                      // 데이터 제거 위치
    struct mutex lock;                  // 커널 mutex
} cmd_queue_t;

/**
 * @brief 원형 큐 초기화 함수
 */
void cmd_queue_init(cmd_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    mutex_init(&q->lock);
}

/**
 * @brief 원형 큐 제거 함수 (자원 정리)
 */
void cmd_queue_destroy(cmd_queue_t *q)
{
    // 커널 mutex는 별도의 해제 필요 없음 (struct mutex는 정적 할당 구조체)
    // 해당 함수는 인터페이스 일관성을 위해 제공 (동적 할당 구조였다면 필요)
}

/**
 * @brief 큐가 비어 있는지 확인 (mutex 보호)
 */
bool cmd_queue_empty(cmd_queue_t *q)
{
    bool empty;

    mutex_lock(&q->lock);
    empty = (q->head == q->tail);
    mutex_unlock(&q->lock);

    return empty;
}

/**
 * @brief 큐가 가득 찼는지 확인 (mutex 보호)
 */
bool cmd_queue_full(cmd_queue_t *q)
{
    bool full;

    mutex_lock(&q->lock);
    full = ((q->head + 1) % CMD_QUEUE_CAPACITY) == q->tail;
    mutex_unlock(&q->lock);

    return full;
}

/**
 * @brief 큐에 데이터 삽입 (mutex 보호)
 * @return 성공 시 true, 실패 시 false
 */
bool cmd_queue_push(cmd_queue_t *q, uint8_t data)
{
    bool success = false;

    mutex_lock(&q->lock);

    if (((q->head + 1) % CMD_QUEUE_CAPACITY) != q->tail) {
        q->buffer[q->head] = data;
        q->head = (q->head + 1) % CMD_QUEUE_CAPACITY;
        success = true;
    }

    mutex_unlock(&q->lock);
    return success;
}

/**
 * @brief 큐에서 데이터 제거 (mutex 보호)
 * @return 성공 시 true, 실패 시 false
 */
bool cmd_queue_pop(cmd_queue_t *q, uint8_t *data)
{
    bool success = false;

    mutex_lock(&q->lock);

    if (q->head != q->tail) {
        *data = q->buffer[q->tail];
        q->tail = (q->tail + 1) % CMD_QUEUE_CAPACITY;
        success = true;
    }

    mutex_unlock(&q->lock);
    return success;
}

#endif