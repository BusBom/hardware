#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEVICE_PATH "/dev/serdev-uart"

void test_write_time_range(int fd, const char *start_hhmm, const char *end_hhmm) {
    char buf[100];
    snprintf(buf, sizeof(buf), "%s:%s\n", start_hhmm, end_hhmm);
    write(fd, buf, strlen(buf));
    printf("Written time range: %s", buf);
}

void test_write_bus_array(int fd) {
    const char *bus_str = "1234:5678:A0B1:M1M2\n";
    write(fd, bus_str, strlen(bus_str));
    printf("Written bus array: %s", bus_str);
}

void test_read_conn_state(int fd) {
    char buf;
    lseek(fd, 0, SEEK_SET);  // 오프셋을 0으로 초기화 (EOF 방지)
    ssize_t r = read(fd, &buf, 1);
    if (r < 0) {
        perror("Read failed");
    } else if (r == 0) {
        printf("No data (EOF)\n");
    } else {
        printf("Connection state: %s\n", buf ? "CONNECTED" : "DISCONNECTED");
    }
}

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    // 시간 설정 테스트
    test_write_time_range(fd, "0830", "2130");

    // 버스 번호 테스트
    sleep(1);  // 드라이버 내부 타이머 동작을 위한 대기
    test_write_bus_array(fd);

    // 연결 상태 읽기 테스트
    sleep(2);
    test_read_conn_state(fd);

    close(fd);
    return EXIT_SUCCESS;
}
