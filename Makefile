# Makefile for CANopenNode with Linux socketCAN (with commander functionalities)


DRV_SRC = .
CANOPEN_SRC = CANopenNode
APPL_SRC = application


LINK_TARGET = canopend


INCLUDE_DIRS = \
	-I$(DRV_SRC) \
	-I$(CANOPEN_SRC) \
	-I$(APPL_SRC)


SOURCES = \
	$(DRV_SRC)/CO_driver.c \
	$(DRV_SRC)/CO_error.c \
	$(DRV_SRC)/CO_epoll_interface.c \
	$(DRV_SRC)/CO_storageLinux.c \
	$(CANOPEN_SRC)/301/CO_ODinterface.c \
	$(CANOPEN_SRC)/301/CO_NMT_Heartbeat.c \
	$(CANOPEN_SRC)/301/CO_HBconsumer.c \
	$(CANOPEN_SRC)/301/CO_Emergency.c \
	$(CANOPEN_SRC)/301/CO_SDOserver.c \
	$(CANOPEN_SRC)/301/CO_SDOclient.c \
	$(CANOPEN_SRC)/301/CO_TIME.c \
	$(CANOPEN_SRC)/301/CO_SYNC.c \
	$(CANOPEN_SRC)/301/CO_PDO.c \
	$(CANOPEN_SRC)/301/crc16-ccitt.c \
	$(CANOPEN_SRC)/301/CO_fifo.c \
	$(CANOPEN_SRC)/303/CO_LEDs.c \
	$(CANOPEN_SRC)/304/CO_GFC.c \
	$(CANOPEN_SRC)/304/CO_SRDO.c \
	$(CANOPEN_SRC)/305/CO_LSSslave.c \
	$(CANOPEN_SRC)/305/CO_LSSmaster.c \
	$(CANOPEN_SRC)/309/CO_gateway_ascii.c \
	$(CANOPEN_SRC)/storage/CO_storage.c \
	$(CANOPEN_SRC)/extra/CO_trace.c \
	$(CANOPEN_SRC)/CANopen.c \
	$(APPL_SRC)/OD.c \
	$(APPL_SRC)/OD_2nd.c \
	$(DRV_SRC)/CO_main_basic.c \
	$(DRV_SRC)/CO_application.c \
	$(DRV_SRC)/agv_queue.c \
	$(DRV_SRC)/agv_pool.c \
	$(DRV_SRC)/agv_log.c \
	$(DRV_SRC)/agv_kinematics.c \
	$(DRV_SRC)/agv_modbus.c \
	$(DRV_SRC)/agv_imu.c


OBJS = $(SOURCES:%.c=%.o)
CC ?= gcc
OPT =
OPT += -g -ggdb
#OPT += -O2
#OPT += -DCO_SINGLE_THREAD=1
#OPT += -DCO_CONFIG_DEBUG=0xFFFF
#OPT += -Wextra -Wshadow -pedantic -fanalyzer
#OPT += -DCO_USE_GLOBALS
OPT += -DCO_MULTIPLE_OD
OPT += -DCO_USE_APPLICATION=1
# P1 心跳容错：启用 HBconsumer，配置取 CO_driver_target.h 的 CO_CONFIG_HB_CONS
# (ENABLE|CALLBACK_CHANGE|QUERY_FUNCT|CALLBACK_PRE|TIMERNEXT|OD_DYNAMIC)
# OPT += -DCO_CONFIG_HB_CONS=0
OPT += -DCO_CONFIG_STORAGE=0
OPT += -Wno-format
CFLAGS = -Wall $(OPT) $(INCLUDE_DIRS)
LDFLAGS =
LDFLAGS += -g -ggdb
LDFLAGS += -pthread
LDFLAGS += -lm

#Options can be also passed via make: 'make OPT="-g" LDFLAGS="-pthread"'


.PHONY: all clean install test clean-test test-asan


# ===== P2 单元测试(零框架, 见 docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html §1.5) =====
# 每个测试是独立可执行文件(自带 main)，只链接被测模块的 .c，绝不碰 CAN/RT。
# 用法:
#   make test                                  # 常规跑
#   make test-asan                             # ASan+UBSan 复核(仅查内存/UB, 不判实时性)
#   make test SAN="-fsanitize=undefined"       # 自定义 sanitizer
SAN =
TEST_CFLAGS  = -Wall -g -O1 -I$(DRV_SRC) -Itests $(SAN)
TEST_LDFLAGS = -pthread -lm $(SAN)

tests/test_agv_queue: tests/test_agv_queue.c tests/agv_test.h $(DRV_SRC)/agv_queue.c
	$(CC) $(TEST_CFLAGS) tests/test_agv_queue.c $(DRV_SRC)/agv_queue.c -o $@ $(TEST_LDFLAGS)

tests/test_agv_odom: tests/test_agv_odom.c tests/agv_test.h $(DRV_SRC)/agv_kinematics.c
	$(CC) $(TEST_CFLAGS) tests/test_agv_odom.c $(DRV_SRC)/agv_kinematics.c -o $@ $(TEST_LDFLAGS)

tests/test_agv_modbus: tests/test_agv_modbus.c tests/agv_test.h $(DRV_SRC)/agv_modbus.c $(DRV_SRC)/agv_modbus.h
	$(CC) $(TEST_CFLAGS) tests/test_agv_modbus.c $(DRV_SRC)/agv_modbus.c -o $@ $(TEST_LDFLAGS)

tests/test_agv_imu: tests/test_agv_imu.c tests/agv_test.h $(DRV_SRC)/agv_imu.c $(DRV_SRC)/agv_imu.h $(DRV_SRC)/agv_modbus.c
	$(CC) $(TEST_CFLAGS) tests/test_agv_imu.c $(DRV_SRC)/agv_imu.c $(DRV_SRC)/agv_modbus.c -o $@ $(TEST_LDFLAGS)

clean-test:
	rm -f tests/test_agv_queue tests/test_agv_odom tests/test_agv_modbus tests/test_agv_imu

test: tests/test_agv_queue tests/test_agv_odom tests/test_agv_modbus tests/test_agv_imu
	@echo "== P2 单元测试 =="
	@tests/test_agv_queue
	@tests/test_agv_odom
	@tests/test_agv_modbus
	@tests/test_agv_imu
	@echo "== 全部通过 =="

test-asan:
	@$(MAKE) --no-print-directory clean-test
	@ASAN_OPTIONS=abort_on_error=1:detect_leaks=1:verify_asan_link_order=0 \
	 $(MAKE) --no-print-directory test SAN="-fsanitize=address,undefined -fno-omit-frame-pointer"
	@$(MAKE) --no-print-directory clean-test   # 清掉 sanitizer 版二进制，避免下次 make test 误用
# 注: verify_asan_link_order=0 用于规避某些环境预加载库导致 ASan 启动报
#     "runtime does not come first"；该开关只关掉链接顺序自检，内存检查照常。


all: clean $(LINK_TARGET)

clean:
	rm -f $(OBJS) $(LINK_TARGET)

install:
	cp $(LINK_TARGET) /usr/bin/$(LINK_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LINK_TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@
