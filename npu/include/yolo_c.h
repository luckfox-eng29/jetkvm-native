#ifndef LUCKFOX_PICO_YOLOV5_YOLO_C_H
#define LUCKFOX_PICO_YOLOV5_YOLO_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
    float conf;
    int cls_id;
} yolo_det_t;

int yolo_init(const char* model_path);
void yolo_deinit();
void yolo_input_shape(int* height, int* width, int* channels);
int yolo_copy_input(const uint8_t* data, size_t len);
int yolo_bind_input_fd(int fd, void* va, size_t size);
int yolo_run(yolo_det_t* dets, int max_dets);
const char* yolo_cls_name(int cls_id);

#ifdef __cplusplus
}
#endif

#endif
