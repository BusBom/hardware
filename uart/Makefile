# 실행 파일 이름들
TARGETS = uart
# 소스 파일 매핑
uart: uart_main.cpp
	g++ -o $@ $^ -lrt

# 기본 빌드 대상
all: $(TARGETS)

# 정리
.PHONY: clean
clean:
	rm -f $(TARGETS)
