#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>

const char* UART_DEVICE_PATH = "/dev/serdev-uart";  // 커널 드라이버에서 등록한 디바이스
std::atomic<bool> running(true);

void receive_thread(int fd) {
    char buf[256];

    while (running) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "\n[UART RECEIVED] " << buf << std::endl;
            std::cout << "> " << std::flush;
        } else {
            usleep(10000);  // 10ms sleep to reduce CPU usage
        }
    }
}

int main() {
    int serial_fd = open(UART_DEVICE_PATH, O_RDWR | O_NONBLOCK);
    if (serial_fd < 0) {
        perror("open UART device");
        return 1;
    }

    std::cout << "UART opened. Type messages to send. Type 'exit' to quit.\n";

    std::thread rx(receive_thread, serial_fd);  // 수신 스레드 시작

    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        input += "\n";  // 개행 추가
        ssize_t written = write(serial_fd, input.c_str(), input.size());
        if (written < 0) {
            perror("write failed");
        }
    }

    running = false;
    rx.join();  // 수신 스레드 종료 대기
    close(serial_fd);

    std::cout << "UART closed.\n";
    return 0;
}
