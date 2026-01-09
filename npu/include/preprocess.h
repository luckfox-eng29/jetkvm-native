#ifndef NPU_PREPROCESS_H
#define NPU_PREPROCESS_H

#include <stdint.h>

void yuyv_to_rgb(const uint8_t* yuyv, uint8_t* rgb, int width, int height);
void resize_rgb_nn(const uint8_t* src, int sw, int sh, uint8_t* dst, int dw, int dh);
int yuyvfd_to_rgb_resized(int src_fd, int sw, int sh, uint8_t* dst_rgb, int dw, int dh);
int yuyvfd_to_rgb_resized_fd(int src_fd, int sw, int sh, int* out_fd, void** out_va, int dw, int dh);
int yuyvfd_to_rgb565_resized(int src_fd, int sw, int sh, uint8_t* dst_rgb, int dw, int dh);

#endif
