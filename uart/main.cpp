#include <iostream>
#include <ctime>    
#include <cstdio>  
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

static int serial_fd = -1;
static char rx_buffer[100];

void handler(int signum);

int uart_open(char* path);
void uart_set(int fd);
int uart_write(int fd, char* msg); //-1 : error, 나머지 실제 읽은 바이트 수
int uart_read(int fd, char* buf); //0: EOF, -1 : error, 나머지 실제 읽은 바이트 수

void now_time(char* buf);

int main(){

    char path[] = "/dev/ttyACM0";
    serial_fd = uart_open(path);
    if(serial_fd == -1) // port not found
        return -1;

    // 2. 시리얼 포트 설정
    uart_set(serial_fd);

    while(1){
        char on[] = "ON:\n";
        char bus[] = "BUS::5678:90AB:CDEF:\n";
        char time[100];
        now_time(time);
        char off[] = "OFF:\n";

        uart_write(serial_fd, on);
        sleep(1);
        uart_write(serial_fd, bus);
        sleep(1);
        uart_write(serial_fd, time);
        sleep(1);
        uart_write(serial_fd, off);
        sleep(1);
    }

    close(serial_fd);
    return 0;
}

void handler(int signum){
    if(serial_fd != -1)
        close(serial_fd);
    serial_fd = -1;
    std::exit(0);
}

int uart_open(char* path){
    int fd = open(path, O_RDWR);
    if(fd < 0){
        std::cout << "Error " << errno << " from open: " << strerror(errno) << "\n";
        return -1;
    };

    return fd;
}

void uart_set(int fd){
    termios tty;

    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error getting termios: " << strerror(errno) << std::endl;
        close(fd);
    }

    // 통신 속도: 9600bps
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    // 8N1 설정
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8비트
    tty.c_iflag &= ~IGNBRK;                         // break 처리 안 함
    tty.c_lflag = 0;                                // no signaling chars, no echo
    tty.c_oflag = 0;                                // no remapping, no delays
    tty.c_cc[VMIN]  = 1;                            // 최소 읽기 바이트 수
    tty.c_cc[VTIME] = 1;                            // 읽기 타임아웃 (0.1초 단위)

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // 소프트웨어 흐름제어 끔
    tty.c_cflag |= (CLOCAL | CREAD);                // 수신 활성화
    tty.c_cflag &= ~(PARENB | PARODD);              // 패리티 없음
    tty.c_cflag &= ~CSTOPB;                         // 1 스톱 비트
    tty.c_cflag &= ~CRTSCTS;                        // 하드웨어 흐름 제어 끔

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting termios: " << strerror(errno) << std::endl;
        close(fd);
    }
}

int uart_write(int fd, char* msg){
    int n_written = write(fd, msg, strlen(msg));
    return n_written;
}


void now_time(char* buf) {
    // 환경 변수 설정: 시간대를 Asia/Seoul로 고정
    setenv("TZ", "Asia/Seoul", 1);
    tzset();  // 시간대 변경 적용

    time_t t = time(nullptr);
    struct tm* now = localtime(&t);

    sprintf(buf, "TIME:%04d:%02d%02d:%02d%02d:\n",
        now->tm_year + 1900,
        now->tm_mon + 1,
        now->tm_mday,
        now->tm_hour,
        now->tm_min
    );

    std::cout << buf << std::endl;
}

int uart_read(int fd, char* buf){
    int n_read = read(fd, buf, sizeof(buf) - 1);
    
    if (n_read > 0) {
        buf[n_read] = '\0';
    }

    return n_read;
}