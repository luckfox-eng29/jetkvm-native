#include "preprocess.h"
#include "../../librga/include/im2d.h"
#include <stddef.h>
#include "../../librga/samples/utils/allocator/include/dma_alloc.h"
#include <string.h>
#include <stdio.h>

static inline int clamp8_local(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static int s_src_fd = -1, s_dst_fd = -1;
static void* s_src_va = NULL;
static void* s_dst_va = NULL;
static int s_w = 0, s_h = 0;
static int s_src_size = 0, s_dst_size = 0;
static int s_cma_disabled = 0;
static int s_mid_fd = -1, s_mid_size = 0;
static void* s_mid_va = NULL;

static void yuyv_to_rgb_cpu(const uint8_t* yuyv, uint8_t* rgb, int width, int height) {
    int stride = width * 2;
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = yuyv + y * stride;
        uint8_t* out = rgb + y * width * 3;
        for (int x = 0; x < width; x += 2) {
            int Y0 = row[x*2+0];
            int U  = row[x*2+1];
            int Y1 = row[x*2+2];
            int V  = row[x*2+3];
            int C0 = Y0 - 16;
            int C1 = Y1 - 16;
            int D = U - 128;
            int E = V - 128;
            int R0 = clamp8_local((298*C0 + 409*E + 128) >> 8);
            int G0 = clamp8_local((298*C0 - 100*E - 208*D + 128) >> 8);
            int B0 = clamp8_local((298*C0 + 516*D + 128) >> 8);
            int R1 = clamp8_local((298*C1 + 409*E + 128) >> 8);
            int G1 = clamp8_local((298*C1 - 100*E - 208*D + 128) >> 8);
            int B1 = clamp8_local((298*C1 + 516*D + 128) >> 8);
            out[0] = (uint8_t)R0; out[1] = (uint8_t)G0; out[2] = (uint8_t)B0;
            out[3] = (uint8_t)R1; out[4] = (uint8_t)G1; out[5] = (uint8_t)B1;
            out += 6;
        }
    }
}

void yuyv_to_rgb(const uint8_t* yuyv, uint8_t* rgb, int width, int height) {
    int w = (width & ~1);
    int src_size = w * height * 2;
    int dst_size = w * height * 3;
    if (s_w != w || s_h != height || s_src_size != src_size || s_dst_size != dst_size || s_src_fd < 0 || s_dst_fd < 0 || s_src_va == NULL || s_dst_va == NULL) {
        if (s_src_fd >= 0) dma_buf_free((size_t)s_src_size, &s_src_fd, s_src_va);
        if (s_dst_fd >= 0) dma_buf_free((size_t)s_dst_size, &s_dst_fd, s_dst_va);
        s_src_fd = -1; s_dst_fd = -1; s_src_va = NULL; s_dst_va = NULL;
        s_w = w; s_h = height; s_src_size = src_size; s_dst_size = dst_size;
        if (s_cma_disabled || dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_src_size, &s_src_fd, &s_src_va) != 0) {
            s_src_fd = -1; s_src_va = NULL;
            s_cma_disabled = 1;
        }
        if (s_cma_disabled || dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_dst_size, &s_dst_fd, &s_dst_va) != 0) {
            s_dst_fd = -1; s_dst_va = NULL;
            s_cma_disabled = 1;
        }
    }
    if (s_src_fd >= 0 && s_dst_fd >= 0 && s_src_va && s_dst_va) {
        memcpy(s_src_va, yuyv, (size_t)s_src_size);
        dma_sync_cpu_to_device(s_src_fd);
        rga_buffer_handle_t src_handle = importbuffer_fd(s_src_fd, &(im_handle_param_t){w, height, RK_FORMAT_YUYV_422});
        rga_buffer_handle_t dst_handle = importbuffer_fd(s_dst_fd, &(im_handle_param_t){w, height, RK_FORMAT_RGB_888});
        if (src_handle && dst_handle) {
            rga_buffer_t src = wrapbuffer_handle(src_handle, w, height, RK_FORMAT_YUYV_422);
            rga_buffer_t dst = wrapbuffer_handle(dst_handle, w, height, RK_FORMAT_RGB_888);
            IM_STATUS ret = imcvtcolor(src, dst, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888, IM_YUV_TO_RGB_BT601_LIMIT);
            if (ret == IM_STATUS_SUCCESS) {
                dma_sync_device_to_cpu(s_dst_fd);
                memcpy(rgb, s_dst_va, (size_t)s_dst_size);
                releasebuffer_handle(src_handle);
                releasebuffer_handle(dst_handle);
                return;
            } else {
                releasebuffer_handle(src_handle);
                releasebuffer_handle(dst_handle);
            }
        }
    }
    yuyv_to_rgb_cpu(yuyv, rgb, w, height);
}

int yuyvfd_to_rgb_resized(int src_fd, int sw, int sh, uint8_t* dst_rgb, int dw, int dh) {
    int w = (sw & ~1);
    int h = sh;
    int mid_size = dw * dh * 2;
    int out_size = dw * dh * 3;
    if (s_mid_fd < 0 || s_mid_size != mid_size || s_mid_va == NULL) {
        if (s_mid_fd >= 0) dma_buf_free((size_t)s_mid_size, &s_mid_fd, s_mid_va);
        s_mid_fd = -1; s_mid_va = NULL; s_mid_size = mid_size;
        if (dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_mid_size, &s_mid_fd, &s_mid_va) != 0) {
            s_mid_fd = -1; s_mid_va = NULL;
        }
    }
    if (s_dst_fd < 0 || s_dst_size != out_size || s_dst_va == NULL) {
        if (s_dst_fd >= 0) dma_buf_free((size_t)s_dst_size, &s_dst_fd, s_dst_va);
        s_dst_fd = -1; s_dst_va = NULL; s_dst_size = out_size;
        if (dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_dst_size, &s_dst_fd, &s_dst_va) != 0) {
            s_dst_fd = -1; s_dst_va = NULL;
        }
    }
    if (s_mid_fd < 0 || s_dst_fd < 0 || s_mid_va == NULL || s_dst_va == NULL) {
        return -1;
    }
    rga_buffer_handle_t src_handle = importbuffer_fd(src_fd, &(im_handle_param_t){w, h, RK_FORMAT_YUYV_422});
    rga_buffer_handle_t mid_handle = importbuffer_fd(s_mid_fd, &(im_handle_param_t){dw, dh, RK_FORMAT_YUYV_422});
    rga_buffer_handle_t out_handle = importbuffer_fd(s_dst_fd, &(im_handle_param_t){dw, dh, RK_FORMAT_RGB_888});
    if (!src_handle || !mid_handle || !out_handle) {
        if (src_handle) releasebuffer_handle(src_handle);
        if (mid_handle) releasebuffer_handle(mid_handle);
        if (out_handle) releasebuffer_handle(out_handle);
        return -1;
    }
    rga_buffer_t src = wrapbuffer_handle(src_handle, w, h, RK_FORMAT_YUYV_422);
    rga_buffer_t mid = wrapbuffer_handle(mid_handle, dw, dh, RK_FORMAT_YUYV_422);
    rga_buffer_t out = wrapbuffer_handle(out_handle, dw, dh, RK_FORMAT_RGB_888);
    IM_STATUS r1 = imresize(src, mid, 0, 0, IM_INTERP_DEFAULT, 1);
    if (r1 != IM_STATUS_SUCCESS) {
        releasebuffer_handle(src_handle);
        releasebuffer_handle(mid_handle);
        releasebuffer_handle(out_handle);
        return -1;
    }
    IM_STATUS r2 = imcvtcolor(mid, out, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888, IM_YUV_TO_RGB_BT601_LIMIT, 1);
    if (r2 != IM_STATUS_SUCCESS) {
        releasebuffer_handle(src_handle);
        releasebuffer_handle(mid_handle);
        releasebuffer_handle(out_handle);
        return -1;
    }
    dma_sync_device_to_cpu(s_dst_fd);
    memcpy(dst_rgb, s_dst_va, (size_t)out_size);
    releasebuffer_handle(src_handle);
    releasebuffer_handle(mid_handle);
    releasebuffer_handle(out_handle);
    return 0;
}

int yuyvfd_to_rgb_resized_fd(int src_fd, int sw, int sh, int* out_fd, void** out_va, int dw, int dh) {
    int w = (sw & ~1);
    int h = sh;
    int mid_size = dw * dh * 2;
    int out_size = dw * dh * 3;
    if (s_mid_fd < 0 || s_mid_size != mid_size || s_mid_va == NULL) {
        if (s_mid_fd >= 0) dma_buf_free((size_t)s_mid_size, &s_mid_fd, s_mid_va);
        s_mid_fd = -1; s_mid_va = NULL; s_mid_size = mid_size;
        if (dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_mid_size, &s_mid_fd, &s_mid_va) != 0) {
            s_mid_fd = -1; s_mid_va = NULL;
        }
    }
    if (s_dst_fd < 0 || s_dst_size != out_size || s_dst_va == NULL) {
        if (s_dst_fd >= 0) dma_buf_free((size_t)s_dst_size, &s_dst_fd, s_dst_va);
        s_dst_fd = -1; s_dst_va = NULL; s_dst_size = out_size;
        if (dma_buf_alloc(RV1106_CMA_HEAP_PATH, (size_t)s_dst_size, &s_dst_fd, &s_dst_va) != 0) {
            s_dst_fd = -1; s_dst_va = NULL;
        }
    }
    if (s_mid_fd < 0 || s_dst_fd < 0 || s_mid_va == NULL || s_dst_va == NULL) {
        return -1;
    }
    rga_buffer_handle_t src_handle = importbuffer_fd(src_fd, &(im_handle_param_t){w, h, RK_FORMAT_YUYV_422});
    rga_buffer_handle_t mid_handle = importbuffer_fd(s_mid_fd, &(im_handle_param_t){dw, dh, RK_FORMAT_YUYV_422});
    rga_buffer_handle_t out_handle = importbuffer_fd(s_dst_fd, &(im_handle_param_t){dw, dh, RK_FORMAT_RGB_888});
    if (!src_handle || !mid_handle || !out_handle) {
        if (src_handle) releasebuffer_handle(src_handle);
        if (mid_handle) releasebuffer_handle(mid_handle);
        if (out_handle) releasebuffer_handle(out_handle);
        return -1;
    }
    rga_buffer_t src = wrapbuffer_handle(src_handle, w, h, RK_FORMAT_YUYV_422);
    rga_buffer_t mid = wrapbuffer_handle(mid_handle, dw, dh, RK_FORMAT_YUYV_422);
    rga_buffer_t out = wrapbuffer_handle(out_handle, dw, dh, RK_FORMAT_RGB_888);
    IM_STATUS r1 = imresize(src, mid, 0, 0, IM_INTERP_DEFAULT, 1);
    if (r1 != IM_STATUS_SUCCESS) {
        releasebuffer_handle(src_handle);
        releasebuffer_handle(mid_handle);
        releasebuffer_handle(out_handle);
        return -1;
    }
    IM_STATUS r2 = imcvtcolor(mid, out, RK_FORMAT_YUYV_422, RK_FORMAT_RGB_888, IM_YUV_TO_RGB_BT601_LIMIT, 1);
    if (r2 != IM_STATUS_SUCCESS) {
        releasebuffer_handle(src_handle);
        releasebuffer_handle(mid_handle);
        releasebuffer_handle(out_handle);
        return -1;
    }
    releasebuffer_handle(src_handle);
    releasebuffer_handle(mid_handle);
    releasebuffer_handle(out_handle);
    if (out_fd) *out_fd = s_dst_fd;
    if (out_va) *out_va = s_dst_va;
    return 0;
}

void resize_rgb_nn(const uint8_t* src, int sw, int sh, uint8_t* dst, int dw, int dh) {
    for (int y = 0; y < dh; ++y) {
        int sy = y * sh / dh;
        for (int x = 0; x < dw; ++x) {
            int sx = x * sw / dw;
            const uint8_t* sp = src + (sy * sw + sx) * 3;
            uint8_t* dp = dst + (y * dw + x) * 3;
            dp[0] = sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
        }
    }
}
