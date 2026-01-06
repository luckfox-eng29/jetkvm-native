#include "yolov5.h"
#include "postprocess.h"
#include "yolo_c.h"
#include <cstring>

static rknn_app_context_t g_ctx;
static bool g_inited = false;

extern "C" {

int yolo_init(const char* model_path) {
    if (g_inited) return 0;
    memset(&g_ctx, 0, sizeof(rknn_app_context_t));
    int ret = init_yolov5_model(model_path, &g_ctx);
    if (ret != 0) return ret;
    ret = init_post_process();
    if (ret != 0) return ret;
    g_inited = true;
    return 0;
}

void yolo_deinit() {
    if (!g_inited) return;
    deinit_post_process();
    release_yolov5_model(&g_ctx);
    memset(&g_ctx, 0, sizeof(rknn_app_context_t));
    g_inited = false;
}

void yolo_input_shape(int* height, int* width, int* channels) {
    if (height) *height = g_ctx.model_height;
    if (width) *width = g_ctx.model_width;
    if (channels) *channels = g_ctx.model_channel;
}

int yolo_copy_input(const uint8_t* data, size_t len) {
    if (!g_inited) return -1;
    size_t need = (size_t)g_ctx.model_height * (size_t)g_ctx.model_width * (size_t)g_ctx.model_channel;
    if (len < need) return -2;
    memcpy(g_ctx.input_mems[0]->virt_addr, data, need);
    int ret = rknn_mem_sync(g_ctx.rknn_ctx, g_ctx.input_mems[0], RKNN_MEMORY_SYNC_TO_DEVICE);
    if (ret != RKNN_SUCC) return ret;
    return 0;
}

int yolo_run(yolo_det_t* dets, int max_dets) {
    if (!g_inited) return -1;
    object_detect_result_list odr;
    int ret = inference_yolov5_model(&g_ctx, &odr);
    if (ret != RKNN_SUCC && ret != 0) return ret;
    int n = odr.count;
    if (n > max_dets) n = max_dets;
    for (int i = 0; i < n; ++i) {
        dets[i].left = odr.results[i].box.left;
        dets[i].top = odr.results[i].box.top;
        dets[i].right = odr.results[i].box.right;
        dets[i].bottom = odr.results[i].box.bottom;
        dets[i].conf = odr.results[i].prop;
        dets[i].cls_id = odr.results[i].cls_id;
    }
    return odr.count;
}

const char* yolo_cls_name(int cls_id) {
    return coco_cls_to_name(cls_id);
}

}
