# mycpu

一个基于 **RV32I** 的简易 CPU 模拟器项目，支持运行裸机程序，并附带几个终端交互示例，其中包括多关卡迷宫游戏。

## 功能简介

这个项目目前包含：

- RV32I 指令集模拟执行
- UART 终端输出
- 键盘输入
- 定时器支持
- 裸机程序加载与运行
- 终端小游戏示例

目前仓库里最有代表性的演示程序是：

- 迷宫游戏（Maze Game）
- 其他基础固件示例（位于 `fw/`）

## 项目结构

```text
mycpu/
├── fw/               # 裸机固件 / 游戏程序
├── src/              # RV32I 模拟器源码
├── build/            # 构建输出目录（生成后出现）
├── CMakeLists.txt    # 模拟器构建配置
├── run_maze.sh       # 一键编译并运行迷宫游戏
└── README.md
环境要求
推荐系统：

Ubuntu / Linux
需要安装的工具：

build-essential
cmake
gcc-riscv64-unknown-elf
binutils-riscv64-unknown-elf
安装依赖
Ubuntu 下可以直接执行：

sudo apt update
sudo apt install -y build-essential cmake gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
获取代码
git clone https://github.com/congxine/mycpu.git
cd mycpu
一键运行迷宫游戏
仓库根目录提供了脚本：

./run_maze.sh
这个脚本会自动完成：

编译模拟器
编译迷宫游戏固件
启动模拟器并运行迷宫游戏
如果脚本没有执行权限，先运行：

chmod +x run_maze.sh
手动构建
1. 构建模拟器
cmake -S . -B build
cmake --build build -j
2. 构建迷宫游戏固件
riscv64-unknown-elf-gcc \
  -march=rv32i -mabi=ilp32 \
  -ffreestanding -nostdlib \
  -O1 -fno-inline \
  -Wl,-T,fw/link.ld \
  -Wl,-Map=fw/maze_game.map \
  -o fw/maze_game.elf \
  fw/start.S fw/maze_game.c
3. 运行
./build/rv32i_sim --raw-kbd fw/maze_game.elf
游戏操作
迷宫游戏操作方式：

w a s d   移动
r         重开当前关
q         退出
n         进入下一关（通关后）
当前迷宫游戏特性
当前版本迷宫游戏支持：

多关卡
计步
计时
胜利 / 失败界面
敌人巡逻
敌人追击状态切换
实时键盘控制
示例说明
运行成功后，你会在终端中看到：

迷宫地图
玩家位置 P
敌人位置 E
目标位置 G
当前回合数 / 时间
敌人状态（如 PATROL / CHASE）
常见问题
1. 找不到 riscv64-unknown-elf-gcc
说明没有安装 RISC-V 裸机交叉编译器，先安装依赖：

sudo apt install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
2. run_maze.sh: Permission denied
给脚本加执行权限：

chmod +x run_maze.sh
3. 终端启动后没有反应
请确认你是在支持标准输入的终端中运行，并使用：

./build/rv32i_sim --raw-kbd fw/maze_game.elf
开发说明
如果你想修改游戏逻辑，可以重点查看：

fw/maze_game.c
如果你想修改模拟器行为，可以重点查看：

src/
后续可扩展方向
这个项目后面还可以继续扩展，例如：

更复杂的敌人 AI
更多关卡
分数系统
道具系统
更完整的外设模拟
更多裸机小游戏
