#include <linux/module.h>
#include <linux/init.h>
#include <linux/serdev.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "serdev-uart"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raspberry Pi Developer");
MODULE_DESCRIPTION("UART serdev driver with string I/O via /dev/serdev-uart");

static struct serdev_device *global_serdev;
static char rx_buffer[256];
static bool is_updated = true;
static size_t rx_size;

/* Receive callback from UART */
static int serdev_uart_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size) {
	size_t to_copy = min(size, sizeof(rx_buffer) - 1);

	memcpy(rx_buffer, buffer, to_copy);
	rx_buffer[to_copy] = '\0';
	rx_size = to_copy;

	pr_info("serdev_echo: Received %zu bytes: %s\n", to_copy, rx_buffer);

	is_updated = false;

	return to_copy;
}

static const struct serdev_device_ops serdev_echo_ops = {
	.receive_buf = serdev_uart_recv,
};

/* Character device: read from rx_buffer */
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
	if (rx_size == 0)
		return 0;

	if(!is_updated){
		if (copy_to_user(buf, rx_buffer, rx_size)){
			is_updated = true;
			return -EFAULT;
		}

		is_updated = true;

		return rx_size;
	}
	
	return 0;
}

/* Character device: write to UART */
static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
	char kbuf[256];
	size_t to_copy = min(count, sizeof(kbuf) - 1);

	if (copy_from_user(kbuf, buf, to_copy))
		return -EFAULT;

	kbuf[to_copy] = '\0';

	pr_info("serdev_echo: Sending %zu bytes: %s\n", to_copy, kbuf);
	serdev_device_write_buf(global_serdev, kbuf, to_copy);

	return to_copy;
}

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

/* serdev probe */
static int serdev_uart_probe(struct serdev_device *serdev) {
	int ret;

	pr_info("serdev_echo: probe called\n");
	global_serdev = serdev;

	serdev_device_set_client_ops(serdev, &serdev_echo_ops);

	ret = serdev_device_open(serdev);
	if (ret) {
		pr_err("serdev_echo: failed to open UART\n");
		return ret;
	}

	serdev_device_set_baudrate(serdev, 115200);
	serdev_device_set_flow_control(serdev, false);
	serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

	ret = misc_register(&echo_miscdev);
	if (ret) {
		pr_err("serdev_echo: failed to register miscdevice\n");
		serdev_device_close(serdev);
		return ret;
	}

	pr_info("serdev_echo: /dev/%s created\n", DEVICE_NAME);
	return 0;
}

static void serdev_uart_remove(struct serdev_device *serdev) {
	pr_info("serdev_echo: remove called\n");
	misc_deregister(&echo_miscdev);
	serdev_device_close(serdev);
	global_serdev = NULL;
}

/* Device tree match */
static const struct of_device_id serdev_echo_ids[] = {
	{ .compatible = "brightlight,echodev" },
	{}
};
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
