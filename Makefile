export LC_ALL=C
SHELL:=/bin/bash

CURRENT_DIR := $(shell pwd)
ifndef LUCKFOX_SDK_PATH
$(error Please Set Luckfox-pico SDK Path. Such as: export LUCKFOX_SDK_PATH=/home/user/luckfox-pico)
endif
RK_SDK_BASE ?= $(LUCKFOX_SDK_PATH)
RK_APP_CROSS := $(RK_SDK_BASE)/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf
RK_MEDIA_OUTPUT := $(RK_SDK_BASE)/media/out
RK_MEDIA_INCLUDE_PATH := $(RK_MEDIA_OUTPUT)/include
RK_APP_MEDIA_LIBS_PATH :=  $(RK_MEDIA_OUTPUT)/lib

RK_APP_LDFLAGS = -L $(RK_APP_MEDIA_LIBS_PATH) -lpthread -lrockit -lrockchip_mpp  -lrga

CC = $(RK_APP_CROSS)-gcc
CXX = $(RK_APP_CROSS)-g++

CFLAGS = -I $(RK_MEDIA_INCLUDE_PATH) -I $(RK_MEDIA_INCLUDE_PATH)/libdrm
CFLAGS += -Wno-int-conversion -Wno-implicit-function-declaration -Wno-discarded-qualifiers
CXXFLAGS = $(CFLAGS) -I $(CURRENT_DIR)/luckfox_pico_yolov5/include -DRV1106_1103
LDFLAGS ?=  -L $(RK_APP_MEDIA_LIBS_PATH) -lpthread -lrockit -lrockchip_mpp -lrga -lm -g -O0 -L $(CURRENT_DIR)/lib -lrknnmrt
BIN 	= kvm_video

#Collect the files to compile
MAINSRC = $(wildcard ./*.c) 
CXXSRC = luckfox_pico_yolov5/src/yolov5.cc luckfox_pico_yolov5/src/postprocess.cc luckfox_pico_yolov5/src/yolo_c_api.cc
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin

OBJEXT 			?= .o

AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

MAINOBJ 		= $(MAINSRC:.c=$(OBJEXT))
CXXOBJ			= $(CXXSRC:.cc=$(OBJEXT))

SRCS 			= $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS 			= $(AOBJS) $(COBJS) $(MAINOBJ) $(CXXOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

all: default

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@ -g -O0
	@echo "CC $<"

$(BUILD_OBJ_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@$(CXX)  $(CXXFLAGS) -c $< -o $@ -g -O0
	@echo "CXX $<"
default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CXX) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean:
	@echo "clean"
	@rm -rf build
