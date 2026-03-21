#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <iostream>

// 前置声明：告诉 CPU 外部有一个叫 Memory 的类，稍后我们会实现它
class Memory; 

class CPU {
private:
    // 1. CPU 的核心资产
    uint32_t regs[32]; // 32 个通用寄存器 (x0 - x31)
    uint32_t pc;       // 程序计数器 (Program Counter，记录当前执行到哪条指令了)
    
    // 2. CPU 的外部联系
    Memory* mem;       // 指向系统内存的指针，CPU 需要通过它去读取指令和读写数据

public:
    // 构造函数：开机时初始化 CPU，把内存条“插”上去
    CPU(Memory* memory);

    // 模拟器运转的三个核心动作
    uint32_t fetch();                       // 步骤一：取指
    void decode_and_execute(uint32_t inst); // 步骤二与三：译码并执行

    // 让 CPU 往前走一步（执行一条指令）
    void step();

    // 调试神器：打印所有寄存器的当前值，方便咱们以后肉眼找 Bug
    void dump_registers();
};

#endif