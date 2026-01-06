cmake_minimum_required(VERSION 3.15)

set(SDK_PATH $ENV{LUCKFOX_SDK_PATH})
if(NOT SDK_PATH)
  message(FATAL_ERROR "Please set LUCKFOX_SDK_PATH to the luckfox-pico SDK path")
  return()
endif()
set(CMAKE_C_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")

project(kvm_video)

link_directories("${SDK_PATH}/media/out/lib")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${SDK_PATH}/media/out/include")
include_directories("${SDK_PATH}/media/out/include/libdrm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/rknn")


set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(kvm_video
  main.c
  video.c
  ctrl.c
  edid.c
  frozen.c
  npu/src/yolov5.cc
  npu/src/postprocess.cc
  npu/src/yolo_c_api.cc
  osd/overlay.c
  npu/preprocess.c
)

target_include_directories(kvm_video PRIVATE
  "${SDK_PATH}/media/out/include"
  "${SDK_PATH}/media/out/include/libdrm"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/rknn"
  "${CMAKE_CURRENT_SOURCE_DIR}/npu/include"
)

target_compile_options(kvm_video PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion -Wno-implicit-function-declaration -Wno-discarded-qualifiers>
  -g
  -O0
)

target_link_directories(kvm_video PRIVATE
  "${SDK_PATH}/media/out/lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

target_link_libraries(kvm_video
  rockit
  rockchip_mpp
  rga
  m
  rknnmrt
)

set_target_properties(kvm_video PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_definitions(kvm_video PRIVATE RV1106_1103)
