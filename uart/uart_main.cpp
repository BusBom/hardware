#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include "json.hpp"

using json = nlohmann::json;

const char* UART_DEVICE_PATH = "/dev/serdev-uart";  // 수정된 디바이스 노드 경로
int serial_fd = -1;

// ---------- UART 관련 ----------
void uart_init(const char* path) {
    serial_fd = open(path, O_WRONLY | O_NONBLOCK);  // write만 사용 중이므로 O_WRONLY
    if (serial_fd < 0) {
        perror("open UART device");
        return;
    }

    std::cout << "UART opened at " << path << std::endl;
}

int uart_write_line(const char* msg) {
    if (serial_fd == -1) return -1;

    ssize_t written = write(serial_fd, msg, strlen(msg));
    if (written < 0) {
        perror("write to UART");
    }
    return written;
}

// ---------- 공유 메모리 관련 ----------
const char* BUS_SHM_NAME = "/busbom_sequence";
const size_t BUS_SHM_SIZE = 4096;

const char* CONFIG_SHM_NAME = "/config_shm";
const size_t CONFIG_SHM_SIZE = 4096;

int bus_shm_fd = -1, config_shm_fd = -1;
char* bus_shm_ptr = nullptr;
char* config_shm_ptr = nullptr;

void init_shared_memory() {
    bus_shm_fd = shm_open(BUS_SHM_NAME, O_RDONLY, 0666);
    config_shm_fd = shm_open(CONFIG_SHM_NAME, O_RDONLY, 0666);

    if (bus_shm_fd < 0 || config_shm_fd < 0) {
        perror("shm_open");
        return;
    }

    bus_shm_ptr = static_cast<char*>(mmap(nullptr, BUS_SHM_SIZE, PROT_READ, MAP_SHARED, bus_shm_fd, 0));
    config_shm_ptr = static_cast<char*>(mmap(nullptr, CONFIG_SHM_SIZE, PROT_READ, MAP_SHARED, config_shm_fd, 0));
}

// ---------- JSON 파싱 및 제어 ----------
bool is_sleep_mode = false;
int frame_counter = 0;

void check_and_send_uart() {
    if (!bus_shm_ptr || !config_shm_ptr) return;

    try {
        json bus_json = json::parse(bus_shm_ptr);
        json config_json = json::parse(config_shm_ptr);

        bool config_sleep_mode = config_json.value("sleep_mode", false);

        if (config_sleep_mode != is_sleep_mode) {
            if (config_sleep_mode) {
                uart_write_line("OFF:\n");
            } else {
                uart_write_line("ON:\n");
            }
            is_sleep_mode = config_sleep_mode;
        }

        // 매 60프레임마다 한 번 버스 정보 전송
        if (!is_sleep_mode && ++frame_counter >= 60) {
            std::string json_str = bus_json.dump();
            if (!json_str.empty()) {
                uart_write_line(json_str.c_str());
                uart_write_line("\n");
            }
            frame_counter = 0;
        }
    } catch (std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

int main() {
    uart_init(UART_DEVICE_PATH);
    if (serial_fd == -1) return -1;

    init_shared_memory();
    if (!bus_shm_ptr || !config_shm_ptr) return -1;

    while (true) {
        check_and_send_uart();
        usleep(1000 * 16);  // 60fps 기준
    }

    return 0;
}
