# BusBom 프로젝트

이 프로젝트는 STM32 펌웨어와 Raspberry Pi 4용 커널 모듈 기반 UART 디바이스 드라이버를 포함합니다. STM32는 STM32CubeIDE 1.18.1로 빌드하며, Raspberry Pi에서는 serdev 기반 드라이버를 통해 UART 통신을 수행합니다.

## STM32 펌웨어 빌드 및 업로드

1. STM32CubeIDE 1.18.1 실행
2. `File` → `Open Projects from File System...` 클릭
3. `Directory`에 `busbom` 폴더 경로 지정 → Finish
4. Project Explorer에서 `busbom` 프로젝트 우클릭 → `Build Project`
5. 빌드 완료 후 보드를 연결
6. 프로젝트 우클릭 → `Run As` → `1 STM32 Cortex-M C/C++ Application` 선택
7. 디버그 설정 창이 뜰 경우 OK 또는 Run 선택

## 라즈베리파이 설정 및 드라이버 설치

### 구동 환경

- 기기: Raspberry Pi 4
- OS: 64bit Legacy (6.1.21-v8+, aarch64)
- UART 핀 연결:
  - GND: 회색 (Pin 6)
  - TX : GPIO14, 보라색 (Pin 8)
  - RX : GPIO15, 파란색 (Pin 10)
 
밑에 안내된 순서대로 수행할 것

### 커널 헤더 설치
```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers
ls /lib/modules/$(uname -r)  # build 폴더 존재 확인
```

### 설정 파일 권한 부여
```bash
sudo chmod +w /boot/config.txt
sudo chmod +w /boot/cmdline.txt
```

### /boot/config.txt에 다음 줄 추가 (sudo 편집기 사용)
```bash
enable_uart=1
dtoverlay=disable-bt
dtoverlay=serdev_overlay
```

### /boot/cmdline.txt에서 다음 항목 제거
```bash
console=serial0,115200
```

### 블루투스 관련 서비스 종료
```bash
sudo systemctl disable hciuart
sudo systemctl stop hciuart
sudo systemctl disable bluetooth
sudo systemctl stop bluetooth
```

### raspi-config 설정
```bash
sudo raspi-config
# Interface Options → Serial Port
# "Login shell over serial" → No
# "Enable serial port hardware" → Yes
sudo reboot
```

### 빌드 및 모듈 적재, 해제
```bash
# 드라이버 소스 다운로드 및 빌드
git clone https://github.com/BusBom/hardware.git
cd hardware/uart/driver
make

# 시간 동기화 및 테스트 입력
sudo timedatectl set-ntp true
timedatectl status  # system clock synchronized: yes 확인
echo "0100:2100" | sudo tee /dev/serdev-uart

# 오버레이 및 커널 모듈 적재
sudo cp serdev_overlay.dtbo /boot/overlays/
sudo insmod serdev_uart.ko
sudo dmesg -c  # "serdev_echo: probe called" 메시지 확인

# 모듈 제거
sudo rmmod serdev_uart
```

### 테스트 실행
```bash
cd ..
make
sudo ./uart
```
정상 출력시 다음과 같음
```bash
Written time range: 0830:2130
Written bus array: 1234:5678:A0B1:M1M2  # B는 음성 없음
Connection state: CONNECTED            # 연결 성공 여부
```
### 드라이버 사용법
write 함수와 read 함수로 사용합니다.
```bash
1) write() - 문자열로 전송
버스 번호 관련 : “PAR1:PAR2:PAR3:PAR4”, 만약 비어 있는 플랫폼이면 공백 문자를 넣어서 보줄 것
ex) “6001:6002:6003:6004”, “6001: :6003:6004”
시간 설정 관련 : 24시간 체계로 보내 줄 것, “START_TIME:END_TIME” 형식으로
ex) “0910:0012”(오전 9시 10분, 오전 12시 12분), “1612:2010”(오후 4시 12분, 오후 8시 10분) 

2) read() - 연결 상태 확인
1 : 연결
0 : 연결 끊어짐
```

