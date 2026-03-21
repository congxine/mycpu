# myCPU 自动化构建脚本

# --- 1. 变量定义区 ---
# C++ 编译器
CXX = g++
CXXFLAGS = -Wall -O2

# RISC-V 交叉编译器
RV_CC = riscv64-unknown-elf-gcc
RV_OBJCOPY = riscv64-unknown-elf-objcopy

# RISC-V 编译参数 (裸机环境、入口地址设定)
RV_CFLAGS = -march=rv32i -mabi=ilp32 -nostdlib -Ttext 0x80000000

# 文件名
SIM_TARGET = myCPU
TEST_SRC = test.S
TEST_ELF = test.elf
TEST_BIN = test.bin
SIM_SRCS = main.cpp cpu.cpp memory.cpp

# --- 2. 构建指令区 ---

# 默认目标：当你只输入 make 时，会同时编译模拟器和汇编代码
all: sim test

# 规则 A：如何编译 C++ 模拟器
sim: $(SIM_SRCS)
	$(CXX) $(CXXFLAGS) $(SIM_SRCS) -o $(SIM_TARGET)
	@echo "[成功] myCPU 模拟器已重新编译！"

# 规则 B：如何编译 RISC-V 测试代码
test: $(TEST_SRC)
	$(RV_CC) $(RV_CFLAGS) $(TEST_SRC) -o $(TEST_ELF)
	$(RV_OBJCOPY) -O binary $(TEST_ELF) $(TEST_BIN)
	@echo "[成功] test.bin 二进制机器码已生成！"

# 规则 C：一键运行 (自动先检查编译，再运行)
run: all
	@echo "\n>>> 开始运行模拟器 >>>"
	./$(SIM_TARGET) $(TEST_BIN)

# 规则 D：打扫卫生，清理所有生成的文件
clean:
	rm -f $(SIM_TARGET) $(TEST_ELF) $(TEST_BIN)
	@echo "[清理] 所有编译产物已删除！"