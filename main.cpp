#include <iostream>
#include <string>
#include "memory.h"
#include "cpu.h"

int main(int argc, char* argv[]) {
    std::cout << "=== 欢迎来到 myCPU 模拟器 ===" << std::endl;

    // 1. 去电脑城买一条 1MB 的内存条
    Memory* mem = new Memory(1024 * 1024);

    // 2. 把程序装进内存
    // 如果你在终端里输入了测试文件的名字 (比如: ./myCPU test.bin)
    if (argc > 1) {
        std::string filename = argv[1];
        if (!mem->load_binary(filename)) {
            std::cerr << "[系统] 找不到测试文件，模拟器已停止。" << std::endl;
            delete mem;
            return 1;
        }
    } else {
        std::cout << "[警告] 你没有放测试文件！" << std::endl;
        // 为了防止程序直接崩溃，我们手动往内存的起始位置 (0x80000000) 
        // 强行塞入一条加法指令: ADD x1, x2, x3 (对应的机器码是 0x003100b3)
        // 假设 x2=0, x3=0, 执行后 x1 也会变成 0，但至少能测试 CPU 的运转
        std::cout << "[系统] 自动注入三条测试指令计算 5+7 ..." << std::endl;
        // 1. ADDI x1, x0, 5  (让 x1 = 0 + 5) -> 机器码: 0x00500093
        mem->write32(0x80000000, 0x00500093);
        // 2. ADDI x2, x0, 7  (让 x2 = 0 + 7) -> 机器码: 0x00700113
        mem->write32(0x80000004, 0x00700113);
        // 3. ADD x3, x1, x2  (让 x3 = x1 + x2) -> 机器码: 0x002081b3
        mem->write32(0x80000008, 0x002081b3);
    }

    // 3. 买一个 CPU，并把内存条插到主板上
    CPU* cpu = new CPU(mem);

    // 4. 按下电源键，启动主循环！
    std::cout << "\n--- 系统启动，CPU 开始执行 ---" << std::endl;
    
    int step_count = 0;
    int max_steps = 5; // 为了防止代码写错陷入死循环，前期调试我们限制它只跑 5 步

    while (step_count < max_steps) {
        std::cout << "\n[Step " << step_count + 1 << "]" << std::endl;
        
        cpu->step();           // 核心动作：取指 -> 译码 -> 执行
        cpu->dump_registers(); // 打印这一步执行完后，所有寄存器的变化
        
        step_count++;
    }

    std::cout << "--- 达到最大步数，系统安全停机 ---" << std::endl;

    // 5. 关机拔电源，释放资源（非常重要，防止内存泄漏）
    delete cpu;
    delete mem;

    return 0;
}