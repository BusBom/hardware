obj-m += serdev_uart.o

all: module dt
	echo Builded Device Tree Overlay and kernel module

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
dt: serdev_overlay.dts
	dtc -@ -I dts -O dtb -o serdev_overlay.dtbo serdev_overlay.dts
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf serdev_overlay.dtbo
	sudo truncate -s 0 /var/log/syslog
	sudo truncate -s 0 /var/log/messages
	sudo truncate -s 0 /var/log/kern.log
	