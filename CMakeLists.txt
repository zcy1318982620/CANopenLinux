# CMakeLists.txt —— 松下 CANopen AGV 工程
# 与根目录 Makefile 等价：同一份源文件清单、宏、头文件路径、链接库。
# 现代 CMake（target_* 风格）：编译参数绑定到具体 target，作用域清晰。

cmake_minimum_required(VERSION 3.16)   # 本机实测 cmake 3.16.3
project(canopend C)                    # 工程名 + 语言（C）

set(CMAKE_C_STANDARD 99)               # 对应 Makefile 无显式标准，这里显式钉 C99
set(CMAKE_C_STANDARD_REQUIRED ON)

# 未指定构建类型时默认 Debug（对应 Makefile 的 -g -ggdb）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug/Release)" FORCE)
endif()

# 找线程库：提供可移植的 Threads::Threads 目标（对应 Makefile 的 -pthread）
# THREADS_PREFER_PTHREAD_FLAG：优先用 -pthread 而非裸 -lpthread（交叉编译更可靠）
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# = Makefile 的 $(LINK_TARGET) + $(SOURCES)，共 31 个源文件
add_executable(canopend
    # —— 驱动/平台层（工程根目录）——
    CO_driver.c
    CO_error.c
    CO_epoll_interface.c
    CO_storageLinux.c

    # —— CANopenNode 协议栈 301 ——
    CANopenNode/301/CO_ODinterface.c
    CANopenNode/301/CO_NMT_Heartbeat.c
    CANopenNode/301/CO_HBconsumer.c
    CANopenNode/301/CO_Emergency.c
    CANopenNode/301/CO_SDOserver.c
    CANopenNode/301/CO_SDOclient.c
    CANopenNode/301/CO_TIME.c
    CANopenNode/301/CO_SYNC.c
    CANopenNode/301/CO_PDO.c
    CANopenNode/301/crc16-ccitt.c
    CANopenNode/301/CO_fifo.c

    # —— CANopenNode 协议栈 303/304/305/309 ——
    CANopenNode/303/CO_LEDs.c
    CANopenNode/304/CO_GFC.c
    CANopenNode/304/CO_SRDO.c
    CANopenNode/305/CO_LSSslave.c
    CANopenNode/305/CO_LSSmaster.c
    CANopenNode/309/CO_gateway_ascii.c

    # —— CANopenNode 存储与工具 ——
    CANopenNode/storage/CO_storage.c
    CANopenNode/extra/CO_trace.c

    # —— CANopenNode 顶层 ——
    CANopenNode/CANopen.c

    # —— 应用层 ——
    application/OD.c
    application/OD_2nd.c
    CO_main_basic.c
    CO_application.c
    agv_queue.c
    agv_pool.c
    agv_log.c
    agv_kinematics.c
    agv_modbus.c
)

# = Makefile 的 $(INCLUDE_DIRS): -I. -ICANopenNode -Iapplication
target_include_directories(canopend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CANopenNode
    ${CMAKE_CURRENT_SOURCE_DIR}/application
)

# = Makefile 的 OPT 里的 -D 宏
# 注意：禁止在此加 CO_CONFIG_HB_CONS=0（P1 心跳容错依赖 HBconsumer，
# 命令行宏会覆盖 CO_driver_target.h 的 #ifndef，静默禁用心跳监控）。
target_compile_definitions(canopend PRIVATE
    CO_MULTIPLE_OD
    CO_USE_APPLICATION=1
    CO_CONFIG_STORAGE=0
)

# = Makefile 的 -Wall -Wno-format；Debug 时额外补 -ggdb
target_compile_options(canopend PRIVATE
    -Wall
    -Wno-format
    $<$<CONFIG:Debug>:-ggdb>
)

# = Makefile 的 LDFLAGS: -pthread -lm
target_link_libraries(canopend PRIVATE
    Threads::Threads
    m
)

# ===== cocomm：ASCII 命令接口客户端 =====
# 对应 cocomm/Makefile：单源文件，-Wall -g -I.，无额外链接库。
add_executable(cocomm cocomm/cocomm.c)

target_include_directories(cocomm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cocomm
)

target_compile_options(cocomm PRIVATE
    -Wall
    $<$<CONFIG:Debug>:-ggdb>
)

# ===== P2 单元测试（零框架，与 Makefile 的 test 目标等价）=====
# 用法:
#   cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
#   cmake -S . -B build -DAGV_SANITIZE=ON   # ASan+UBSan（仅查内存/UB，不判实时性，见 doc §3.3）
enable_testing()
option(AGV_SANITIZE "启用 ASan+UBSan 编译测试(慢, 改变时序)" OFF)

set(AGV_TEST_CFLAGS -Wall -g -O1)
if(AGV_SANITIZE)
    list(APPEND AGV_TEST_CFLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

# —— 队列单测：只链接 agv_queue.c ——
add_executable(test_agv_queue tests/test_agv_queue.c agv_queue.c)
target_include_directories(test_agv_queue PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_options(test_agv_queue PRIVATE ${AGV_TEST_CFLAGS})
target_link_libraries(test_agv_queue PRIVATE Threads::Threads m)
if(AGV_SANITIZE)
    target_link_options(test_agv_queue PRIVATE -fsanitize=address,undefined)
endif()

# —— 里程计/逆解单测：只链接 agv_kinematics.c ——
add_executable(test_agv_odom tests/test_agv_odom.c agv_kinematics.c)
target_include_directories(test_agv_odom PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_options(test_agv_odom PRIVATE ${AGV_TEST_CFLAGS})
target_link_libraries(test_agv_odom PRIVATE m)
if(AGV_SANITIZE)
    target_link_options(test_agv_odom PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME agv_queue COMMAND test_agv_queue)
add_test(NAME agv_odom  COMMAND test_agv_odom)

# —— Modbus RTU 主站单测：只链接 agv_modbus.c（纯函数，不碰串口）——
add_executable(test_agv_modbus tests/test_agv_modbus.c agv_modbus.c)
target_include_directories(test_agv_modbus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_options(test_agv_modbus PRIVATE ${AGV_TEST_CFLAGS})
target_link_libraries(test_agv_modbus PRIVATE m)
if(AGV_SANITIZE)
    target_link_options(test_agv_modbus PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME agv_modbus COMMAND test_agv_modbus)
