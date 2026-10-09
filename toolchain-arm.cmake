# toolchain-arm.cmake —— 交叉编译到 i.MX6ULL（ARM 32-bit / hard-float）
#
# 用法：
#   cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
#   cmake --build build-arm -j16
#
# 等价于用 Makefile 时的：make CC=arm-linux-gnueabihf-gcc

# 目标平台：交叉编译必须设 SYSTEM_NAME，否则 CMake 会当成主机原生编译
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Linaro 4.9.4 交叉工具链（本机实测路径，见 HANDOFF.md）
set(TOOLCHAIN_PREFIX /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}/bin/arm-linux-gnueabihf-g++)

# 工具链自带 sysroot：让 find_* 只在目标根目录里找库/头文件，不去主机乱找
set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_PREFIX}/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
