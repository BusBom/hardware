# 실행 파일 이름들
TARGETS = uart bus cam

# 소스 파일 매핑
uart: uart_main.cpp
	g++ -o $@ $^ -lrt

bus: busbom_sequence_test.cpp
	g++ -o $@ $^ -lrt

cam: camera_config_test.cpp
	g++ -o $@ $^ -lrt

sender: simple_sender.cpp
	g++ -o $@ $^ -lrt -pthread

# 기본 빌드 대상
all: $(TARGETS)

# 정리
.PHONY: clean
clean:
	rm -f $(TARGETS)
	sudo rm /dev/shm/busbom_sequence
	sudo rm /dev/shm/camera_config
