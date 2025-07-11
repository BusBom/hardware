#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include "json.hpp"

using json = nlohmann::json;

const char *SHM_NAME = "/busbom_sequence";
const size_t SHM_SIZE = 4096;

int main() {
    // JSON 생성 예시
    json j = json::array({
        {
            {"platform", 1},
            {"status", "approaching"},
            {"busNumber", "1111"}
        },
        {
            {"platform", 2},
            {"status", "approaching"},
            {"busNumber", "2222"}
        },
        {
            {"platform", 4},
            {"status", "approaching"},
            {"busNumber", "4444"}
        }
    });

    std::string data = j.dump(); // JSON 직렬화

    // 공유 메모리 생성 및 열기
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // 크기 설정
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    // 매핑
    void* ptr = mmap(nullptr, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // 쓰기
    std::memset(ptr, 0, SHM_SIZE); // 초기화
    std::memcpy(ptr, data.c_str(), data.size());

    // 정리
    munmap(ptr, SHM_SIZE);
    close(shm_fd);

    std::cout << "✅ bus sequence shared memory에 JSON 쓰기 완료\n";
    return 0;
}
