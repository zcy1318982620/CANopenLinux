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
	$(DRV_SRC)/agv_log.c


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
OPT += -DCO_CONFIG_HB_CONS=0
OPT += -DCO_CONFIG_STORAGE=0
OPT += -Wno-format
CFLAGS = -Wall $(OPT) $(INCLUDE_DIRS)
LDFLAGS =
LDFLAGS += -g -ggdb
LDFLAGS += -pthread
LDFLAGS += -lm

#Options can be also passed via make: 'make OPT="-g" LDFLAGS="-pthread"'


.PHONY: all clean

all: clean $(LINK_TARGET)

clean:
	rm -f $(OBJS) $(LINK_TARGET)

install:
	cp $(LINK_TARGET) /usr/bin/$(LINK_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LINK_TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@
