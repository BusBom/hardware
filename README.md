# BusBom Project Installation and Usage Manual

## 1. Overview

The **BusBom Project** consists of two main components for UART-based communication:

- **STM32 firmware** (built using STM32CubeIDE 1.18.1)
- **Raspberry Pi 4 kernel module-based UART device driver** (using serdev)

### System Structure

- STM32 functions as the UART transmitter, Raspberry Pi as the receiver.
- Uses the serdev interface for kernel-level communication.

## 2. System Requirements

| Item | Specification |
|------|---------------|
| MCU | STM32 series microcontroller |
| Development Tool | STM32CubeIDE 1.18.1 |
| SBC | Raspberry Pi 4 (64bit) |
| OS | Raspberry Pi OS 64bit Legacy (Kernel 6.1.21-v8+) |
| Communication | UART (GPIO14: TX, GPIO15: RX) |
| Kernel Driver | serdev-based kernel module (manual build and load) |

---

## 3. STM32 Firmware Build and Upload

1. Launch **STM32CubeIDE**.
2. Select `File` → `Open Projects from File System...`.
3. Set `Directory` to the `busbom` project folder path → Finish.
4. In **Project Explorer**, right-click on the `busbom` project → `Build Project`.
5. Connect the STM32 board.
6. Right-click on the project → `Run As` → `1 STM32 Cortex-M C/C++ Application`.  
   If the debug configuration window appears, click OK or Run.

---

## 4. Raspberry Pi Setup and Driver Installation

### 4.1 UART Hardware Connection

| Raspberry Pi Pin | Signal | Example Color |
|------------------|--------|---------------|
| Pin 6            | GND    | Gray          |
| Pin 8 (GPIO14)   | TX     | Purple        |
| Pin 10 (GPIO15)  | RX     | Blue          |

### 4.2 Install Packages and Kernel Headers
```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers
ls /lib/modules/$(uname -r)
# Ensure the 'build' folder exists
```

### 4.3 Edit Configuration Files
```bash
sudo chmod +w /boot/config.txt
sudo chmod +w /boot/cmdline.txt
```

#### Add to `/boot/config.txt`:
```bash
enable_uart=1
dtoverlay=disable-bt
dtoverlay=serdev_overlay
```

#### Remove from `/boot/cmdline.txt`:
```bash
console=serial0,115200
```

### 4.4 Disable Bluetooth
```bash
sudo systemctl disable hciuart
sudo systemctl stop hciuart
sudo systemctl disable bluetooth
sudo systemctl stop bluetooth
```

### 4.5 Configure Serial Port with `raspi-config`
```bash
sudo raspi-config
# Interface Options → Serial Port
# "Login shell over serial" → No
# "Enable serial port hardware" → Yes
sudo reboot
```

---

## 5. Build and Load the Kernel Module

### 5.1 Download and Build Driver Source
```bash
git clone https://github.com/BusBom/hardware.git
cd hardware/uart/driver
make
```

### 5.2 Enable Time Synchronization
```bash
sudo timedatectl set-ntp true
timedatectl status
# Check that 'System clock synchronized: yes'
```

### 5.3 Load Kernel Module and Test
```bash
sudo cp serdev_overlay.dtbo /boot/overlays/
sudo insmod serdev_uart.ko
sudo dmesg -c
# Look for "serdev_echo: probe called"
```

### 5.4 Remove Module (if needed)
```bash
sudo rmmod serdev_uart
```

---

## 6. Run the Test Program
```bash
cd ..
make
sudo ./uart
```

### Example Output
```bash
Written time range: 0830:2130
Written bus array: 1234:5678:A0B1:M1M2
Connection state: CONNECTED
```

---

## 7. Driver Usage

### `write()` Function

- **Set bus numbers**: `"BUS1:BUS2:BUS3:BUS4"`  
  Use a space character (`' '`) for empty platforms.  
  Example: `"6001: :6003:6004"`

- **Set operating time**: `"START_TIME:END_TIME"` (24-hour format)  
  Example: `"0910:0012"` → 9:10 AM to 12:12 AM  
  Example: `"1612:2010"` → 4:12 PM to 8:10 PM

```bash
echo "0100:2100" | sudo tee /dev/serdev-uart
echo "6001:6002:6003:6004" | sudo tee /dev/serdev-uart
```

### `read()` Function

- **Check connection status**:
  - `1` : Connected
  - `0` : Disconnected

```bash
cat /dev/serdev-uart
```

⚠ **Note**: Initial connection may take some time.

---

## 8. Reference Diagrams

- **STM32 Firmware Architecture**  
  ![Firmware Architecture](https://github.com/user-attachments/assets/3153f47e-b32e-4c6c-af3c-683003207166)

- **Raspberry Pi Driver Architecture**  
  ![Driver Architecture](https://github.com/user-attachments/assets/4243f8b9-81a4-403b-8107-130ecb626cf6)

---

## 9. Troubleshooting

| Symptom | Cause and Solution |
|---------|--------------------|
| No `serdev-uart` device | Check `/boot/config.txt` and kernel module settings |
| No "probe called" message | Verify with `dmesg` after `insmod` |
| `cat /dev/serdev-uart` returns 0 | Check STM32 connection or TX/RX wiring |
| `write()` has no effect | Format error or `/dev/serdev-uart` permission issue |
