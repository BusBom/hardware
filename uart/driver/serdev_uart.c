/*
sudo truncate -s 0 /var/log/syslog
sudo truncate -s 0 /var/log/messages
sudo truncate -s 0 /var/log/kern.log
*/
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/rtc.h>
#include <linux/sched.h>
#include <linux/serdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>

#include "cmd_queue.h"

#define DEVICE_NAME "serdev-uart"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raspberry Pi Developer");
MODULE_DESCRIPTION("UART serdev driver with string I/O via /dev/serdev-uart");

static struct serdev_device *global_serdev;

/*uart */
static DEFINE_MUTEX(rx_buffer_mtx);
static char rx_buffer[256];
static size_t rx_size;

/*timer */
static struct hrtimer main_timer;  // timer for sending command
static ktime_t main_period;
static struct hrtimer sub_timer;  // timer for clock and on/off
static ktime_t sub_period;

#define MAX_STR_SIZE 100
#define MAX_PLATFORM_SIZE 4

/*mutex variable*/
static DEFINE_MUTEX(conn_state_mtx);
static bool conn_state = false;        // true : connected, false : unconnected
static DEFINE_MUTEX(onoff_state_mtx);  // true : on, false : off
static bool onoff_state = false;
static DEFINE_MUTEX(skip_clock_mtx);
static bool skip_clock = false;
static DEFINE_MUTEX(bus_array_mtx);
static char bus_array[MAX_PLATFORM_SIZE][MAX_STR_SIZE];
static DEFINE_MUTEX(time_config_array_mtx);
static struct timespec64 time_config_array[2];  // local time base

/*declared functions*/
static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos);
static ssize_t dev_read(struct file *file, char __user *buf, size_t count,
                        loff_t *ppos);

static struct task_struct *uart_txrx_thread;
static cmd_queue_t uart_cmd_queue;
static int uart_txrx_thread_fn(void *data);
static enum hrtimer_restart main_timer_callback(struct hrtimer *timer);
static enum hrtimer_restart sub_timer_callback(struct hrtimer *timer);
static int parse(char *src, char (*dest)[100], char delimeter, int max_tokens);
static int fill_timespec64_with_hhmm(const char *hhmm_str,
                                     struct timespec64 *ts_out);
static void format_local_timespec64_to_string(
    const struct timespec64 *ts,  // debuging
    char *buf, size_t buf_size);

static char prev_bus_array[MAX_PLATFORM_SIZE][MAX_STR_SIZE];
static bool is_new_bus_array(void);
static void get_clock_cmd(char *buf, size_t buf_size);

static int serdev_uart_recv(struct serdev_device *serdev,
                            const unsigned char *buffer, size_t size);
static int serdev_uart_probe(struct serdev_device *serdev);
static void serdev_uart_remove(struct serdev_device *serdev);

static const struct serdev_device_ops serdev_echo_ops = {
    .receive_buf = serdev_uart_recv,
};

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static struct miscdevice echo_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fops,
    .mode = 0666,
};

/* Device tree match */
static const struct of_device_id serdev_echo_ids[] = {
    {.compatible = "brightlight,echodev"}, {}};
MODULE_DEVICE_TABLE(of, serdev_echo_ids);

/* Register driver */
static struct serdev_device_driver serdev_echo_driver = {
    .probe = serdev_uart_probe,
    .remove = serdev_uart_remove,
    .driver =
        {
            .name = "serdev-uart-driver",
            .of_match_table = serdev_echo_ids,
        },
};

module_serdev_device_driver(serdev_echo_driver);

/*function define */
/*fwrite function*/
static char last_input_from_user[MAX_STR_SIZE] = "";
static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos) {
  char kbuf[MAX_STR_SIZE] = {
      0};  // {'\0',} 보다 {0} 이 더 일반적인 초기화 방식입니다.
  char parsed_str[MAX_PLATFORM_SIZE][MAX_STR_SIZE];
  size_t to_copy;

  // '\0' 공간을 확보하기 위해 count와 (버퍼크기 - 1) 중 작은 값을 선택합니다.
  to_copy = min(count, sizeof(kbuf) - 1);

  if (copy_from_user(kbuf, buf, to_copy)) return -EFAULT;

  // kbuf는 이미 0으로 초기화되었으므로 별도의 NULL 종단 처리가 필요 없습니다.
  // 사용자가 전달한 데이터에 개행 문자가 있다면 제거합니다.
  if (to_copy > 0 && kbuf[to_copy - 1] == '\n') {
    kbuf[to_copy - 1] = '\0';
  }

  // 이전에 들어온 값과 동일하면 아무 작업도 하지 않고 성공을 반환합니다.
  if (strcmp(kbuf, last_input_from_user) == 0) {
    // 성공적으로 '쓴' 바이트 수인 to_copy를 반환하는 것이 더 올바릅니다.
    return to_copy;
  }

  // 새로운 입력을 last_input_from_user에 복사합니다.
  strncpy(last_input_from_user, kbuf, sizeof(last_input_from_user) - 1);
  // strncpy는 항상 NULL로 끝나지 않을 수 있으므로, 마지막을 확실히 NULL로
  // 설정합니다.
  last_input_from_user[sizeof(last_input_from_user) - 1] = '\0';

  int parsed_count = parse(kbuf, parsed_str, ':', MAX_PLATFORM_SIZE);

  if (parsed_count == 2) {
    // 요청하신 대로 mutex_lock으로 변경
    mutex_lock(&time_config_array_mtx);

    if (fill_timespec64_with_hhmm(parsed_str[0], &time_config_array[0]) < 0 ||
        fill_timespec64_with_hhmm(parsed_str[1], &time_config_array[1]) < 0) {
      mutex_unlock(&time_config_array_mtx);
      return -EINVAL;
    }

    // end time이 start time보다 이전이거나 같으면 하루를 더함
    if (time_config_array[1].tv_sec <= time_config_array[0].tv_sec) {
      time_config_array[1].tv_sec += 86400;  // 24시간을 초로 환산
    }

    mutex_unlock(&time_config_array_mtx);

    // --- 디버깅 출력부 수정 ---
    struct tm st, et;
    long offset_sec = -sys_tz.tz_minuteswest * 60;  // 시스템 시간대 오프셋(초)

    // 저장된 UTC 타임스탬프를 시스템 로컬 시간으로 변환하여 출력
    time64_to_tm(time_config_array[0].tv_sec, offset_sec, &st);
    time64_to_tm(time_config_array[1].tv_sec, offset_sec, &et);

    pr_info("Formatted time start: %04ld-%02d-%02d %02d:%02d\n",
            st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour,
            st.tm_min);

    pr_info("Formatted time end  : %04ld-%02d-%02d %02d:%02d\n",
            et.tm_year + 1900, et.tm_mon + 1, et.tm_mday, et.tm_hour,
            et.tm_min);
  } else if (parsed_count == 4) {
    // 요청하신 대로 mutex_lock으로 변경
    mutex_lock(&bus_array_mtx);

    // ⚠️ [중요] memcpy 대신 strncpy를 사용하여 버퍼
    // 오버플로우를 방지하고 NULL 종단을 보장합니다.
    for (int i = 0; i < MAX_PLATFORM_SIZE; i++) {
      strncpy(bus_array[i], parsed_str[i], MAX_STR_SIZE - 1);
      bus_array[i][MAX_STR_SIZE - 1] = '\0';  // NULL 종단 보장

      printk("BUS: %s", bus_array[i]);
    }

    mutex_unlock(&bus_array_mtx);
  }

  // 성공 시 사용자가 요청한 count를 반환하는 것이 일반적입니다.
  return count;
}

/*fread function*/
static ssize_t dev_read(struct file *file, char __user *buf, size_t count,
                        loff_t *ppos) {
  char sig_val[2] = {
      0,
  };
  ssize_t retval = 0;

  // 사용자가 요청한 count가 1바이트보다 작으면 읽을 수 없음
  if (count < 1) {
    return -EINVAL;  // Invalid argument 에러 반환
  }

  if (mutex_lock_interruptible(&conn_state_mtx)) return -ERESTARTSYS;

  // 파일 오프셋(*ppos)이 0보다 크면 이미 데이터를 읽어간 것임
  // EOF(End-Of-File)를 의미하는 0을 반환
  if (*ppos > 0) {
    mutex_unlock(&conn_state_mtx);
    return 0;  // EOF
  }

  if (conn_state) {
    sig_val[0] = 1;
  } else {
    sig_val[0] = 0;
  }

  // 사용자 공간으로 1바이트 복사
  // copy_to_user는 실패 시 0이 아닌 값을 반환
  if (copy_to_user(buf, &sig_val, 1)) {
    retval = -EFAULT;  // Bad address 에러 반환
  } else {
    // 성공 시, 읽은 바이트 수(1)를 반환값으로 설정
    retval = 1;
    // 파일 오프셋을 1 증가시켜 다음 read 호출 시에는 읽지 않도록 함
    *ppos += 1;
  }

  mutex_unlock(&conn_state_mtx);

  return retval;
}

/*uart tx-rx thread function */
static int uart_txrx_thread_fn(void *data) {
  cmd_queue_init(&uart_cmd_queue);

  uint8_t cmd;
  bool is_daily_updated = false;

  while (!kthread_should_stop()) {
    if (cmd_queue_pop(&uart_cmd_queue, &cmd)) {
      switch (cmd) {
        case CONN: {
          // printk("CONN\n");
          bool new_conn_state = false;  // 지역 변수로 상태를 먼저 판단
          for (int i = 0; i < 5; i++) {
            serdev_device_write_buf(global_serdev, "CONN:\n",
                                    strlen("CONN:\n"));
            msleep(1);  // busy-wait 대신 msleep 사용

            mutex_lock(&rx_buffer_mtx);
            if (rx_buffer[0] == '1') {
              new_conn_state = true;
              mutex_unlock(&rx_buffer_mtx);
              break;  // 연결 성공 시 즉시 루프 탈출
            }
            mutex_unlock(&rx_buffer_mtx);
          }

          // 최종적으로 결정된 상태를 한 번만 lock해서 업데이트
          mutex_lock(&conn_state_mtx);
          conn_state = new_conn_state;
          mutex_unlock(&conn_state_mtx);
          break;
        }
        case BUS: {
          // printk("BUS\n");
          if (is_new_bus_array()) {
            char cmd[100] = "BUS:";
            int cmd_index = 4, bus_num_len = 0;

            mutex_lock(&bus_array_mtx);
            for (int i = 0; i < MAX_PLATFORM_SIZE; i++) {
              strncpy(prev_bus_array[i], bus_array[i], sizeof(bus_array[i]));
            }
            mutex_unlock(&bus_array_mtx);

            for (int i = 0; i < MAX_PLATFORM_SIZE; i++) {
              bus_num_len = strlen(prev_bus_array[i]);
              strncpy(cmd + cmd_index, prev_bus_array[i], bus_num_len);
              cmd[cmd_index + bus_num_len] = ':';
              cmd_index += (bus_num_len + 1);
            }
            cmd[cmd_index++] = '\n';
            cmd[cmd_index] = '\0';

            serdev_device_write_buf(global_serdev, cmd, cmd_index);

            mutex_lock(&skip_clock_mtx);
            skip_clock = true;
            mutex_unlock(&skip_clock_mtx);
          }
          break;
        }
        case CLOCK: {
          // printk("CLOCK\n");
          char cmd[100];
          get_clock_cmd(cmd, 100);
          printk("CLOCK : %s\n", cmd);
          serdev_device_write_buf(global_serdev, cmd, strlen(cmd));
          break;
        }
        case ONOFF: {
          struct timespec64 current_utc_time;
          ktime_get_real_ts64(&current_utc_time);
          struct timespec64 current_kst_time = current_utc_time;
          current_kst_time.tv_sec += 32400;

          mutex_lock(&time_config_array_mtx);

          if (timespec64_compare(&current_kst_time, &time_config_array[1]) >=
              0) {
            printk("CMD:OFF\n");
            serdev_device_write_buf(global_serdev, "OFF:\n", strlen("OFF:\n"));

            mutex_lock(&bus_array_mtx);
            for (int i = 0; i < MAX_PLATFORM_SIZE; i++) {
              memset(bus_array[i], '\0', sizeof(bus_array[i]));
            }
            mutex_unlock(&bus_array_mtx);

            mutex_lock(&onoff_state_mtx);
            onoff_state = false;
            is_daily_updated = false;
            printk("%s\n", onoff_state ? "true" : "false");
            mutex_unlock(&onoff_state_mtx);

            time_config_array[0].tv_sec += 86400;
            time_config_array[1].tv_sec += 86400;

            char buf[2][100];
            format_local_timespec64_to_string(&time_config_array[0], buf[0],
                                              100);
            format_local_timespec64_to_string(&time_config_array[1], buf[1],
                                              100);
            printk("UPDT:%s\n", buf[0]);
            printk("UPDT:%s\n", buf[1]);

          } else if (!is_daily_updated &&
                     timespec64_compare(&current_kst_time,
                                        &time_config_array[0]) >= 0) {
            printk("CMD:ON\n");
            serdev_device_write_buf(global_serdev, "ON:\n", strlen("ON:\n"));
            mutex_lock(&onoff_state_mtx);
            onoff_state = true;
            is_daily_updated = true;
            printk("%s\n", onoff_state ? "true" : "false");
            mutex_unlock(&onoff_state_mtx);
          }
          mutex_unlock(&time_config_array_mtx);

          break;
        }
      }
    } else {
      msleep(1);
    }
  }

  return 0;
}

// 추천: main_timer_callback
static enum hrtimer_restart main_timer_callback(struct hrtimer *timer) {
  cmd_queue_push(&uart_cmd_queue, CONN);

  mutex_lock(&conn_state_mtx);   // 1. conn 잠금
  mutex_lock(&onoff_state_mtx);  // 2. onoff 잠금

  if (conn_state && onoff_state) {
    cmd_queue_push(&uart_cmd_queue, BUS);
  }

  // 잠근 순서의 역순으로 해제 (onoff -> conn)
  mutex_unlock(&onoff_state_mtx);  // 2. onoff 해제
  mutex_unlock(&conn_state_mtx);   // 1. conn 해제

  hrtimer_forward_now(timer, main_period);
  return HRTIMER_RESTART;
}

// 추천: sub_timer_callback
static enum hrtimer_restart sub_timer_callback(struct hrtimer *timer) {
  mutex_lock(&conn_state_mtx);   // 1. conn 잠금
  mutex_lock(&onoff_state_mtx);  // 2. onoff 잠금

  if (conn_state) {
    cmd_queue_push(&uart_cmd_queue, ONOFF);
    mutex_lock(&skip_clock_mtx);  // 3. skip 잠금
    if (onoff_state && !skip_clock) {
      cmd_queue_push(&uart_cmd_queue, CLOCK);
    } else {
      skip_clock = false;
    }
    mutex_unlock(&skip_clock_mtx);  // 3. skip 해제
  }

  // 잠근 순서의 역순으로 해제 (onoff -> conn)
  mutex_unlock(&onoff_state_mtx);  // 2. onoff 해제
  mutex_unlock(&conn_state_mtx);   // 1. conn 해제

  hrtimer_forward_now(timer, sub_period);
  return HRTIMER_RESTART;
}

/* Receive callback from UART */
static int serdev_uart_recv(struct serdev_device *serdev,
                            const unsigned char *buffer, size_t size) {
  mutex_lock(&rx_buffer_mtx);
  size_t to_copy = min(size, sizeof(rx_buffer) - 1);
  memcpy(rx_buffer, buffer, to_copy);
  rx_buffer[to_copy] = '\0';
  rx_size = to_copy;
  mutex_unlock(&rx_buffer_mtx);
  // pr_info("serdev_echo: Received %zu bytes: %s\n", to_copy, rx_buffer);

  return to_copy;
}

/* serdev probe */
static int serdev_uart_probe(struct serdev_device *serdev) {
  int ret;

  // pr_info("serdev_echo: probe called\n");
  global_serdev = serdev;

  serdev_device_set_client_ops(serdev, &serdev_echo_ops);

  ret = serdev_device_open(serdev);
  if (ret) {
    pr_err("serdev_echo: failed to open UART\n");
    return ret;
  }

  // uart driver
  serdev_device_set_baudrate(serdev, 115200);
  serdev_device_set_flow_control(serdev, false);
  serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

  // timer setting
  main_period = ktime_set(1, 0);
  sub_period = ktime_set(30, 0);  // 60, 2 is for debugging

  hrtimer_init(&main_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  main_timer.function = main_timer_callback;
  hrtimer_start(&main_timer, main_period, HRTIMER_MODE_REL);

  hrtimer_init(&sub_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  sub_timer.function = sub_timer_callback;
  hrtimer_start(&sub_timer, sub_period, HRTIMER_MODE_REL);

  uart_txrx_thread = kthread_run(uart_txrx_thread_fn, NULL, "uart_txrx_thread");
  if (IS_ERR(uart_txrx_thread)) {
    pr_err("kthread_driver: failed to create thread\n");
    return PTR_ERR(uart_txrx_thread);
  }

  ret = misc_register(&echo_miscdev);
  if (ret) {
    pr_err("serdev_echo: failed to register miscdevice\n");
    serdev_device_close(serdev);
    return ret;
  }

  serdev_device_write_buf(global_serdev, "OFF:\n", strlen("OFF:\n"));
  // pr_info("serdev_echo: /dev/%s created\n", DEVICE_NAME);
  return 0;
}

static void serdev_uart_remove(struct serdev_device *serdev) {
  // pr_info("serdev_echo: remove called\n");

  if (uart_txrx_thread) {
    kthread_stop(uart_txrx_thread);  // 쓰레드 종료 요청
    uart_txrx_thread = NULL;
  }

  hrtimer_cancel(&main_timer);
  hrtimer_cancel(&sub_timer);

  misc_deregister(&echo_miscdev);
  serdev_device_close(serdev);
  global_serdev = NULL;
}

/*parser*/
static int parse(char *src, char (*dest)[100], char delimiter, int max_tokens) {
  char *token;
  int index = 0;
  char delim[2] = {delimiter, '\0'};

  while ((token = strsep(&src, delim)) != NULL && index < max_tokens) {
    strscpy(dest[index], token, 100);
    index++;
  }

  return index;
}

static int fill_timespec64_with_hhmm(const char *hhmm_str,
                                     struct timespec64 *ts) {
  if (!hhmm_str || !ts) return -EINVAL;

  int hour, min;
  if (sscanf(hhmm_str, "%2d%2d", &hour, &min) != 2) return -EINVAL;
  if (hour < 0 || hour > 23 || min < 0 || min > 59) return -EINVAL;

  struct timespec64 now_ts;
  struct tm tm_local;
  time64_t local_sec;

  // 1. 현재 UTC 시간을 가져옴
  ktime_get_real_ts64(&now_ts);

  // 2. 시스템 시간대 오프셋을 적용하여 현재 '로컬 시간'을 초 단위로 계산
  //    sys_tz.tz_minuteswest는 그리니치 서쪽 기준 분 단위 오프셋입니다.
  //    KST(UTC+9)의 경우 -540이므로, UTC - (-540 * 60) = UTC + 9시간이 됩니다.
  local_sec = now_ts.tv_sec - (sys_tz.tz_minuteswest * 60);

  // 3. 계산된 로컬 시간을 tm 구조체로 변환
  time64_to_tm(local_sec, 0, &tm_local);

  // 4. 로컬 시간의 년/월/일과 사용자 입력 시/분을 사용하여 UTC 타임스탬프 생성
  //    mktime64는 주어진 시간 요소들을 시스템 로컬 시간으로 간주하고,
  //    이를 UTC 타임스탬프(time64_t)로 변환해줍니다.
  ts->tv_sec = mktime64(tm_local.tm_year + 1900, tm_local.tm_mon + 1,
                        tm_local.tm_mday, hour, min, 0);
  ts->tv_nsec = 0;

  return 0;
}

static bool is_new_bus_array(void) {
  bool changed = false;
  int i;

  mutex_lock(&bus_array_mtx);
  for (i = 0; i < MAX_PLATFORM_SIZE; i++) {
    if (strcmp(bus_array[i], prev_bus_array[i]) != 0) {
      changed = true;
      // 루프를 즉시 멈추고 임계 영역을 빠져나가도록 break 사용
      break;
    }
  }
  mutex_unlock(&bus_array_mtx);  // 임계 영역이 끝난 후 한번만 unlock

  return changed;
}

static void get_clock_cmd(char *buf, size_t buf_size) {
  struct timespec64 ts;
  struct tm tm;

  // 1. UTC 기준 현재 시간 얻기
  ktime_get_real_ts64(&ts);  // ts.tv_sec: 초 단위 시간

  // 2. local time 보정: sys_tz은 offset (분 단위)
  ts.tv_sec += 32400;

  // 3. 초 단위 시간을 struct tm으로 변환 (UTC에서 보정된 값)
  time64_to_tm(ts.tv_sec, 0,
               &tm);  // korean time으로 고정(다른 나라면 시간 보정 필요)

  // 4. 문자열 포맷 구성: TIME:YYYY:MMDD:hhmm\n
  snprintf(buf, buf_size, "TIME:%04ld:%02d%02d:%02d%02d:\n",
           (long)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
           tm.tm_min);
}

// for debugging
static void format_local_timespec64_to_string(const struct timespec64 *ts,
                                              char *buf, size_t buf_size) {
  struct tm tm;

  // ts는 이미 local time (예: KST) 기준이므로 보정 없이 바로 변환
  time64_to_tm(ts->tv_sec, 0, &tm);

  snprintf(buf, buf_size, "%04ld년 %02d월 %02d일 %02d:%02d\n",
           (long)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
           tm.tm_min);
}
