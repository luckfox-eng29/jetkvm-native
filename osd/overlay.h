#ifndef OSD_OVERLAY_H
#define OSD_OVERLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "yolo_c.h"

int overlay_init(int width, int height, int venc_channel);
int overlay_deinit(int venc_channel);
int overlay_draw_detections(int model_w, int model_h, const yolo_det_t* dets, int count, uint32_t color);

#endif
