/*
sudo truncate -s 0 /var/log/syslog
sudo truncate -s 0 /var/log/messages
sudo truncate -s 0 /var/log/kern.log
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/serdev.h>
#include <linux/kthread.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "serdev-uart"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raspberry Pi Developer");
MODULE_DESCRIPTION("UART serdev driver with string I/O via /dev/serdev-uart");

static struct serdev_device *global_serdev;

/*uart */
static char rx_buffer[256];
static bool is_updated = true;
static size_t rx_size;

/*timer */
static struct hrtimer main_timer; // timer for sending command
static ktime_t main_period;
static struct hrtimer sub_timer; // timer for clock and on/off
static ktime_t sub_period;

#define MAX_STR_SIZE 100
#define MAX_PLATFORM_SIZE 4

/*mutex variable*/
static DEFINE_MUTEX(conn_mtx);
static bool conn_state = false; // true : connected, false : unconnected
static DEFINE_MUTEX(onoff_mtx); // true : on, false : off
static bool onoff_state = false;
static DEFINE_MUTEX(is_there_bus_mtx);
static bool is_there_bus = false;
static DEFINE_MUTEX(bus_array_mtx);
static char bus_array[MAX_PLATFORM_SIZE][MAX_STR_SIZE];
static DEFINE_MUTEX(time_config_array_mtx);
static struct timespec64 time_config_array[2]; // local time base

/*declared functions*/
static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);

static int uart_txrx_thread_fn(void); static struct task_struct *uart_txrx_thread;
static enum hrtimer_restart main_timer_callback(struct hrtimer *timer);
static enum hrtimer_restart sub_timer_callback(struct hrtimer *timer);
static int parse(char *src, char (*dest)[100], char delimeter, int max_tokens);
static int fill_timespec64_with_hhmm(const char *hhmm_str, struct timespec64 *ts_out);
static unsigned long millis(void);
static void ms_delay(unsigned int ms);
static bool is_new_bus_array(void);

static int serdev_uart_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size);
static ssize_t serdev_uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t serdev_uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
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
	{.compatible = "brightlight,echodev"},
	{}};
MODULE_DEVICE_TABLE(of, serdev_echo_ids);

/* Register driver */
static struct serdev_device_driver serdev_echo_driver = {
	.probe = serdev_uart_probe,
	.remove = serdev_uart_remove,
	.driver = {
		.name = "serdev-uart-driver",
		.of_match_table = serdev_echo_ids,
	},
};

module_serdev_device_driver(serdev_echo_driver);

/*function define */
/*fwrite function*/
static char last_input_from_user[MAX_STR_SIZE] = "";
static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char kbuf[MAX_STR_SIZE] = {0}; // {'\0',} 보다 {0} 이 더 일반적인 초기화 방식입니다.
	char parsed_str[MAX_PLATFORM_SIZE][MAX_STR_SIZE];
	size_t to_copy;

	// '\0' 공간을 확보하기 위해 count와 (버퍼크기 - 1) 중 작은 값을 선택합니다.
	to_copy = min(count, sizeof(kbuf) - 1);

	if (copy_from_user(kbuf, buf, to_copy))
		return -EFAULT;

	// kbuf는 이미 0으로 초기화되었으므로 별도의 NULL 종단 처리가 필요 없습니다.
	// 사용자가 전달한 데이터에 개행 문자가 있다면 제거합니다.
	if (to_copy > 0 && kbuf[to_copy - 1] == '\n')
	{
		kbuf[to_copy - 1] = '\0';
	}

	// 이전에 들어온 값과 동일하면 아무 작업도 하지 않고 성공을 반환합니다.
	if (strcmp(kbuf, last_input_from_user) == 0)
	{
		// 성공적으로 '쓴' 바이트 수인 to_copy를 반환하는 것이 더 올바릅니다.
		return to_copy;
	}

	// 새로운 입력을 last_input_from_user에 복사합니다.
	strncpy(last_input_from_user, kbuf, sizeof(last_input_from_user) - 1);
	// strncpy는 항상 NULL로 끝나지 않을 수 있으므로, 마지막을 확실히 NULL로 설정합니다.
	last_input_from_user[sizeof(last_input_from_user) - 1] = '\0';

	int parsed_count = parse(kbuf, parsed_str, ':', MAX_PLATFORM_SIZE);

	if (parsed_count == 2)
	{
		// 요청하신 대로 mutex_lock으로 변경
		mutex_lock(&time_config_array_mtx);

		if (fill_timespec64_with_hhmm(parsed_str[0], &time_config_array[0]) < 0 ||
			fill_timespec64_with_hhmm(parsed_str[1], &time_config_array[1]) < 0)
		{
			mutex_unlock(&time_config_array_mtx);
			return -EINVAL;
		}

		// end time이 start time보다 이전이거나 같으면 하루를 더함
		if (time_config_array[1].tv_sec <= time_config_array[0].tv_sec)
		{
			time_config_array[1].tv_sec += 86400; // 24시간을 초로 환산
		}

		mutex_unlock(&time_config_array_mtx);

		// --- 디버깅 출력부 수정 ---
		/* struct tm st, et;
		 long offset_sec = -sys_tz.tz_minuteswest * 60; // 시스템 시간대 오프셋(초)

		 // 저장된 UTC 타임스탬프를 시스템 로컬 시간으로 변환하여 출력
		 time64_to_tm(time_config_array[0].tv_sec, offset_sec, &st);
		 time64_to_tm(time_config_array[1].tv_sec, offset_sec, &et);

		 // pr_info("Formatted time start: %04ld-%02d-%02d %02d:%02d\n",
				 st.tm_year + 1900, st.tm_mon + 1, st.tm_mday,
				 st.tm_hour, st.tm_min);

		 // pr_info("Formatted time end  : %04ld-%02d-%02d %02d:%02d\n",
				 et.tm_year + 1900, et.tm_mon + 1, et.tm_mday,
				 et.tm_hour, et.tm_min);*/
	}
	else if (parsed_count == 4)
	{
		// 요청하신 대로 mutex_lock으로 변경
		mutex_lock(&bus_array_mtx);

		// ⚠️ [중요] memcpy 대신 strncpy를 사용하여 버퍼 오버플로우를 방지하고 NULL 종단을 보장합니다.
		strncpy(bus_array[0], parsed_str[0], MAX_STR_SIZE - 1);
		strncpy(bus_array[1], parsed_str[1], MAX_STR_SIZE - 1);
		strncpy(bus_array[2], parsed_str[2], MAX_STR_SIZE - 1);
		strncpy(bus_array[3], parsed_str[3], MAX_STR_SIZE - 1);

		mutex_unlock(&bus_array_mtx);
	}

	// 성공 시 사용자가 요청한 count를 반환하는 것이 일반적입니다.
	return count;
}

/*fread function*/
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char sig_val;
	ssize_t retval = 0;

	// 사용자가 요청한 count가 1바이트보다 작으면 읽을 수 없음
	if (count < 1)
	{
		return -EINVAL; // Invalid argument 에러 반환
	}

	if (mutex_lock_interruptible(&conn_mtx))
		return -ERESTARTSYS;

	// 파일 오프셋(*ppos)이 0보다 크면 이미 데이터를 읽어간 것임
	// EOF(End-Of-File)를 의미하는 0을 반환
	if (*ppos > 0)
	{
		mutex_unlock(&conn_mtx);
		return 0; // EOF
	}

	if (conn_state)
	{
		sig_val = '1';
	}
	else
	{
		sig_val = '0';
	}

	// 사용자 공간으로 1바이트 복사
	// copy_to_user는 실패 시 0이 아닌 값을 반환
	if (copy_to_user(buf, &sig_val, 1))
	{
		retval = -EFAULT; // Bad address 에러 반환
	}
	else
	{
		// 성공 시, 읽은 바이트 수(1)를 반환값으로 설정
		retval = 1;
		// 파일 오프셋을 1 증가시켜 다음 read 호출 시에는 읽지 않도록 함
		*ppos += 1;
	}

	mutex_unlock(&conn_mtx);

	return retval;
}

/*uart tx-rx thread function */
//need int type queue
static int uart_txrx_thread_fn(void){

}

/*main timer callback*/
static char prev_bus_input[MAX_PLATFORM_SIZE][MAX_STR_SIZE];

static bool is_new_bus_array()
{
	mutex_lock(&bus_array_mtx);
	for (int i = 0; i < MAX_PLATFORM_SIZE; i++)
	{
		if (strcmp(bus_array[i], prev_bus_input[i]) != 0)
		{
			return true;
		}
	}
	mutex_lock(&bus_array_mtx);
	return false;
}

static enum hrtimer_restart main_timer_callback(struct hrtimer *timer)
{
	/*connection */
	unsigned long ms = millis();
	mutex_lock(&conn_mtx);
	while (millis() - ms < 50)
	{
		serdev_device_write_buf(global_serdev, "CONN:\n", strlen("CONN:\n"));
		ms_delay(5);
		if (strlen(rx_buffer) == 1 && rx_buffer[0] == '1')
		{

			if (rx_buffer[0] == '1')
			{
				conn_state = true;
				break;
			}
			else
			{
				conn_state = false;
			}
		}
		ms_delay(5);
		ms = millis();
	}
	mutex_unlock(&conn_mtx);

	if (conn_state)
	{
		if (is_new_bus_array())
		{
			mutex_lock(&bus_array_mtx);
			for (int i = 0; i < MAX_PLATFORM_SIZE; i++)
			{
				strncpy(prev_bus_input[i], bus_array[i], sizeof(prev_bus_input[i]));
				prev_bus_input[i][sizeof(prev_bus_input[i]) - 1] = '\0';
			}
			mutex_unlock(&bus_array_mtx);

			char cmd_buf[MAX_STR_SIZE] = "BUS:";
			int len = strlen(cmd_buf);
			for (int i = 0; i < MAX_PLATFORM_SIZE; i++)
			{
				int ret = snprintf(cmd_buf + len, MAX_STR_SIZE - len, "%s:", prev_bus_input[i]);
				if (ret < 0 || ret >= MAX_STR_SIZE - len)
					break;
				len += ret;
			}
			cmd_buf[len] = '\n'; // 마지막 ':' → '\n'으로 바꿈

			serdev_device_write_buf(global_serdev, cmd_buf, index + 1);

			mutex_lock(&is_there_bus_mtx);
			is_there_bus = true;
			mutex_unlock(&is_there_bus_mtx);
		}
		else
		{
			mutex_lock(&is_there_bus_mtx);
			is_there_bus = false;
			mutex_unlock(&is_there_bus_mtx);
		}
	}

	hrtimer_forward_now(timer, main_period); // 주기 설정
	return HRTIMER_RESTART;
}

/*sub timer callback*/
static enum hrtimer_restart sub_timer_callback(struct hrtimer *timer)
{

	// printk("sub timer");

	hrtimer_forward_now(timer, sub_period); // 주기 설정
	return HRTIMER_RESTART;
}

/* Receive callback from UART */
static int serdev_uart_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size)
{
	size_t to_copy = min(size, sizeof(rx_buffer) - 1);

	memcpy(rx_buffer, buffer, to_copy);
	rx_buffer[to_copy] = '\0';
	rx_size = to_copy;

	// pr_info("serdev_echo: Received %zu bytes: %s\n", to_copy, rx_buffer);

	is_updated = false;

	return to_copy;
}

/* Character device: read from rx_buffer */
static ssize_t serdev_uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	if (rx_size == 0)
		return 0;

	if (!is_updated)
	{
		if (copy_to_user(buf, rx_buffer, rx_size))
		{
			is_updated = true;
			return -EFAULT;
		}

		is_updated = true;

		return rx_size;
	}

	return 0;
}

/* Character device: write to UART */
static ssize_t serdev_uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char kbuf[256];
	size_t to_copy = min(count, sizeof(kbuf) - 1);

	if (copy_from_user(kbuf, buf, to_copy))
		return -EFAULT;

	kbuf[to_copy] = '\0';

	// pr_info("serdev_echo: Sending %zu bytes: %s\n", to_copy, kbuf);
	serdev_device_write_buf(global_serdev, kbuf, to_copy);

	return to_copy;
}

/* serdev probe */
static int serdev_uart_probe(struct serdev_device *serdev)
{
	int ret;

	// pr_info("serdev_echo: probe called\n");
	global_serdev = serdev;

	serdev_device_set_client_ops(serdev, &serdev_echo_ops);

	ret = serdev_device_open(serdev);
	if (ret)
	{
		pr_err("serdev_echo: failed to open UART\n");
		return ret;
	}

	// uart driver
	serdev_device_set_baudrate(serdev, 115200);
	serdev_device_set_flow_control(serdev, false);
	serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

	// timer setting
	main_period = ktime_set(1, 0);
	sub_period = ktime_set(2, 0); // 60, 2 is for debugging

	hrtimer_init(&main_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	main_timer.function = main_timer_callback;
	hrtimer_start(&main_timer, main_period, HRTIMER_MODE_REL);

	hrtimer_init(&sub_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sub_timer.function = sub_timer_callback;
	hrtimer_start(&sub_timer, sub_period, HRTIMER_MODE_REL);

	uart_txrx_thread = kthread_run(uart_txrx_thread_fn, NULL, "uart_txrx_thread");
	if (IS_ERR(uart_txrx_thread)) {
		pr_err("kthread_driver: failed to create thread\n");
		return PTR_ERR(my_thread);
	}

	ret = misc_register(&echo_miscdev);
	if (ret)
	{
		pr_err("serdev_echo: failed to register miscdevice\n");
		serdev_device_close(serdev);
		return ret;
	}

	// pr_info("serdev_echo: /dev/%s created\n", DEVICE_NAME);
	return 0;
}

static void serdev_uart_remove(struct serdev_device *serdev)
{
	// pr_info("serdev_echo: remove called\n");

	if (uart_txrx_thread) {
		kthread_stop(uart_txrx_thread);  // 쓰레드 종료 요청
		my_thread = NULL;
	}

	hrtimer_cancel(&main_timer);
	hrtimer_cancel(&sub_timer);

	misc_deregister(&echo_miscdev);
	serdev_device_close(serdev);
	global_serdev = NULL;
}

/*parser*/
static int parse(char *src, char (*dest)[100], char delimiter, int max_tokens)
{
	char *token;
	int index = 0;
	char delim[2] = {delimiter, '\0'};

	while ((token = strsep(&src, delim)) != NULL && index < max_tokens)
	{
		strscpy(dest[index], token, 100);
		index++;
	}

	return index;
}

static int fill_timespec64_with_hhmm(const char *hhmm_str, struct timespec64 *ts)
{
	if (!hhmm_str || !ts)
		return -EINVAL;

	int hour, min;
	if (sscanf(hhmm_str, "%2d%2d", &hour, &min) != 2)
		return -EINVAL;
	if (hour < 0 || hour > 23 || min < 0 || min > 59)
		return -EINVAL;

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
	ts->tv_sec = mktime64(tm_local.tm_year + 1900, tm_local.tm_mon + 1, tm_local.tm_mday, hour, min, 0);
	ts->tv_nsec = 0;

	return 0;
}

static unsigned long millis(void)
{
	return jiffies_to_msecs(jiffies);
}

static void ms_delay(unsigned int ms)
{
	unsigned long now = millis();
	while (millis() - now < ms)
		;
}
