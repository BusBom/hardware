#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "json.hpp"
using json = nlohmann::json;

const char* BUS_SHM_NAME = "/busbom_sequence";
const size_t BUS_SHM_SIZE = 4096;

const char* CONFIG_SHM_NAME = "/camera_config";
const size_t CONFIG_SHM_SIZE = 4096;

bool on_triggered_today = false;
bool off_triggered_today = false;

// UART
static int serial_fd = -1;
char buses[4][100] = { "" };

// function declarations
void uart_init(const char* path);
int uart_write_line(const char* msg);
void now_time(char* buf);
bool read_sequence(char (*buses)[100]);
bool process_sleep_mode();
bool is_time_passed(const std::string& ref_time, const struct tm* now);
bool parse_json_from_shm(const char* shm_name, size_t shm_size, json& out_json);

int main() {
    uart_init("/dev/ttyACM0");
    if (serial_fd == -1) return -1;

    uart_write_line("ON:\n");
    std::time_t last_print_time = std::time(nullptr);
    int screen_type = 0;

    while (true) {
        if (std::time(nullptr) - last_print_time >= 60) {
            
            process_sleep_mode();
            
            if (screen_type == 0) {
                char tx_buffer[100] = "";
                now_time(tx_buffer);
                uart_write_line(tx_buffer);
            } else {
                read_sequence(buses);
                std::string msg = "BUS:";
                for (int i = 0; i < 4; ++i)
                    msg += std::string(buses[i]) + ":";
                msg += "\n";
                uart_write_line(msg.c_str());
            }

            screen_type = (screen_type + 1) % 3;
            last_print_time = std::time(nullptr);
        }

        sleep(1);
    }

    close(serial_fd);
    return 0;
}

// ---------- UART 관련 ----------
void uart_init(const char* path) {
    serial_fd = open(path, O_RDWR);
    if (serial_fd < 0) {
        perror("open UART");
        return;
    }

    termios tty = {};
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("tcgetattr");
        close(serial_fd);
        serial_fd = -1;
        return;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK);
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(serial_fd);
        serial_fd = -1;
    }
}

int uart_write_line(const char* msg) {
    if (serial_fd == -1) return -1;
    return write(serial_fd, msg, strlen(msg));
}

// ---------- 시간 관련 ----------
void now_time(char* buf) {
    setenv("TZ", "Asia/Seoul", 1);
    tzset();
    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    sprintf(buf, "TIME:%04d:%02d%02d:%02d%02d:\n",
            now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
            now->tm_hour, now->tm_min);
}

bool is_time_passed(const std::string& ref_time, const struct tm* now) {
    int h = std::stoi(ref_time.substr(0, 2));
    int m = std::stoi(ref_time.substr(3, 2));
    return (now->tm_hour > h) || (now->tm_hour == h && now->tm_min >= m);
}

// ---------- 슬립모드 처리 ----------
bool process_sleep_mode() {
    time_t raw = time(nullptr);
    struct tm* now = localtime(&raw);

    if (now->tm_hour == 0 && now->tm_min == 0) {
        on_triggered_today = false;
        off_triggered_today = false;
    }

    json config;
    if (!parse_json_from_shm(CONFIG_SHM_NAME, CONFIG_SHM_SIZE, config)) return false;

    if (!config.contains("sleepMode") || !config["sleepMode"]["enabled"].get<bool>())
        return false;

    const std::string& startTime = config["sleepMode"]["startTime"];
    const std::string& endTime   = config["sleepMode"]["endTime"];

    if (!on_triggered_today && is_time_passed(startTime, now)) {
        uart_write_line("ON:\n");
        on_triggered_today = true;
    }

    if (!off_triggered_today && is_time_passed(endTime, now)) {
        uart_write_line("OFF:\n");
        off_triggered_today = true;
    }

    return true;
}

// ---------- 버스 정보 처리 ----------
bool read_sequence(char (*buses)[100]) {
    json bus_data;
    if (!parse_json_from_shm(BUS_SHM_NAME, BUS_SHM_SIZE, bus_data)) return false;

    bool changed = false;
    for (const auto& item : bus_data) {
        int platform = item.value("platform", -1);
        std::string number = item.value("busNumber", "");

        if (platform >= 1 && platform <= 4) {
            if (strcmp(buses[platform - 1], number.c_str()) != 0) {
                strncpy(buses[platform - 1], number.c_str(), sizeof(buses[0]) - 1);
                buses[platform - 1][sizeof(buses[0]) - 1] = '\0';
                changed = true;
            }
        }
    }

    return changed;
}

// ---------- 공통 JSON 파서 ----------
bool parse_json_from_shm(const char* shm_name, size_t shm_size, json& out_json) {
    int fd = shm_open(shm_name, O_RDONLY, 0666);
    if (fd == -1) {
        perror("shm_open");
        return false;
    }

    void* ptr = mmap(nullptr, shm_size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return false;
    }

    try {
        std::string json_str(static_cast<char*>(ptr));
        out_json = json::parse(json_str);
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        munmap(ptr, shm_size);
        close(fd);
        return false;
    }

    munmap(ptr, shm_size);
    close(fd);
    return true;
}
