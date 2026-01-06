#include <string.h>
#include <stdio.h>
#include <rk_mpi_rgn.h>
#include "overlay.h"

static RGN_HANDLE g_osd = 0;
static bool g_osd_inited = false;
static int g_osd_w = 0, g_osd_h = 0;
static int g_venc_channel = 0;

static void osd_clear(uint32_t* buf, uint32_t vir_w, uint32_t vir_h) {
    uint32_t total = vir_w * vir_h;
    for (uint32_t i = 0; i < total; ++i) buf[i] = 0x00000000;
}

static void osd_draw_rect(uint32_t* buf, uint32_t vir_w, uint32_t vir_h, int x0, int y0, int x1, int y1, int thick, uint32_t color) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int)vir_w) x1 = (int)vir_w - 1;
    if (y1 >= (int)vir_h) y1 = (int)vir_h - 1;
    for (int t = 0; t < thick; ++t) {
        int yt = y0 + t;
        if (yt <= y1) {
            uint32_t* row = buf + (uint32_t)yt * vir_w;
            for (int x = x0; x <= x1; ++x) row[x] = color;
        }
        int yb = y1 - t;
        if (yb >= y0) {
            uint32_t* row = buf + (uint32_t)yb * vir_w;
            for (int x = x0; x <= x1; ++x) row[x] = color;
        }
        int xl = x0 + t;
        for (int y = y0; y <= y1; ++y) {
            uint32_t* row = buf + (uint32_t)y * vir_w;
            if (xl <= x1) row[xl] = color;
        }
        int xr = x1 - t;
        for (int y = y0; y <= y1; ++y) {
            uint32_t* row = buf + (uint32_t)y * vir_w;
            if (xr >= x0) row[xr] = color;
        }
    }
}

static const uint8_t font8x8[128][8] = {
    [' '] = {0,0,0,0,0,0,0,0},
    ['-'] = {0,0,0,0x1E,0,0,0,0},
    ['.'] = {0,0,0,0,0,0x18,0x18,0},
    ['0'] = {0x18,0x24,0x2C,0x34,0x24,0x24,0x18,0},
    ['1'] = {0x08,0x18,0x08,0x08,0x08,0x08,0x3E,0},
    ['2'] = {0x1C,0x22,0x02,0x04,0x08,0x10,0x3E,0},
    ['3'] = {0x1C,0x22,0x02,0x0C,0x02,0x22,0x1C,0},
    ['4'] = {0x04,0x0C,0x14,0x24,0x3E,0x04,0x04,0},
    ['5'] = {0x3E,0x20,0x3C,0x02,0x02,0x22,0x1C,0},
    ['6'] = {0x0C,0x10,0x20,0x3C,0x22,0x22,0x1C,0},
    ['7'] = {0x3E,0x02,0x04,0x08,0x10,0x10,0x10,0},
    ['8'] = {0x1C,0x22,0x22,0x1C,0x22,0x22,0x1C,0},
    ['9'] = {0x1C,0x22,0x22,0x1E,0x02,0x04,0x18,0},
    ['A'] = {0x18,0x24,0x24,0x3C,0x24,0x24,0x24,0},
    ['B'] = {0x38,0x24,0x24,0x38,0x24,0x24,0x38,0},
    ['C'] = {0x1C,0x20,0x20,0x20,0x20,0x20,0x1C,0},
    ['D'] = {0x38,0x24,0x24,0x24,0x24,0x24,0x38,0},
    ['E'] = {0x3C,0x20,0x20,0x38,0x20,0x20,0x3C,0},
    ['F'] = {0x3C,0x20,0x20,0x38,0x20,0x20,0x20,0},
    ['G'] = {0x1C,0x20,0x20,0x2C,0x24,0x24,0x1C,0},
    ['H'] = {0x24,0x24,0x24,0x3C,0x24,0x24,0x24,0},
    ['I'] = {0x1C,0x08,0x08,0x08,0x08,0x08,0x1C,0},
    ['J'] = {0x0C,0x04,0x04,0x04,0x24,0x24,0x18,0},
    ['K'] = {0x24,0x28,0x30,0x20,0x30,0x28,0x24,0},
    ['L'] = {0x20,0x20,0x20,0x20,0x20,0x20,0x3C,0},
    ['M'] = {0x24,0x3C,0x3C,0x24,0x24,0x24,0x24,0},
    ['N'] = {0x24,0x34,0x34,0x2C,0x2C,0x24,0x24,0},
    ['O'] = {0x18,0x24,0x24,0x24,0x24,0x24,0x18,0},
    ['P'] = {0x38,0x24,0x24,0x38,0x20,0x20,0x20,0},
    ['Q'] = {0x18,0x24,0x24,0x24,0x24,0x28,0x18,0x04},
    ['R'] = {0x38,0x24,0x24,0x38,0x30,0x28,0x24,0},
    ['S'] = {0x1C,0x20,0x20,0x18,0x04,0x04,0x38,0},
    ['T'] = {0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0},
    ['U'] = {0x24,0x24,0x24,0x24,0x24,0x24,0x18,0},
    ['V'] = {0x24,0x24,0x24,0x24,0x14,0x18,0x08,0},
    ['W'] = {0x24,0x24,0x24,0x3C,0x3C,0x3C,0x24,0},
    ['X'] = {0x24,0x14,0x18,0x08,0x18,0x14,0x24,0},
    ['Y'] = {0x22,0x14,0x14,0x08,0x08,0x08,0x08,0},
    ['Z'] = {0x3C,0x04,0x08,0x10,0x20,0x20,0x3C,0},
};

static void osd_draw_char(uint32_t* buf, uint32_t vir_w, uint32_t vir_h, int x, int y, char c, uint32_t color, int scale) {
    unsigned char uc = (unsigned char)c;
    const uint8_t* pat = font8x8[uc];
    int w = 8, h = 8;
    for (int ry = 0; ry < h; ++ry) {
        uint8_t row = pat[ry];
        for (int rx = 0; rx < w; ++rx) {
            if (row & (1 << (7 - rx))) {
                for (int sy = 0; sy < scale; ++sy) {
                    int py = y + ry*scale + sy;
                    if (py < 0 || py >= (int)vir_h) continue;
                    uint32_t* prow = buf + (uint32_t)py * vir_w;
                    for (int sx = 0; sx < scale; ++sx) {
                        int px = x + rx*scale + sx;
                        if (px < 0 || px >= (int)vir_w) continue;
                        prow[px] = color;
                    }
                }
            }
        }
    }
}

static void osd_draw_text(uint32_t* buf, uint32_t vir_w, uint32_t vir_h, int x, int y, const char* text, uint32_t color, int scale) {
    int cx = x;
    int len = (int)strlen(text);
    for (int i = 0; i < len; ++i) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        osd_draw_char(buf, vir_w, vir_h, cx, y, c, color, scale);
        cx += 8 * scale;
    }
}

int overlay_init(int width, int height, int venc_channel) {
    if (g_osd_inited) return RK_SUCCESS;
    g_venc_channel = venc_channel;
    RGN_ATTR_S stRgnAttr;
    memset(&stRgnAttr, 0, sizeof(stRgnAttr));
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = (PIXEL_FORMAT_E)RK_FMT_ARGB8888;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width = width;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = height;
    stRgnAttr.unAttr.stOverlay.u32ClutNum = 0;
    RK_S32 ret = RK_MPI_RGN_Create(g_osd, &stRgnAttr);
    if (ret != RK_SUCCESS) {
        return ret;
    }
    RGN_CHN_ATTR_S stRgnChnAttr;
    memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
    stRgnChnAttr.bShow = RK_TRUE;
    stRgnChnAttr.enType = OVERLAY_RGN;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bEnable = RK_FALSE;
    stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bForceIntra = RK_TRUE;
    stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = RK_FALSE;
    stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp = RK_FALSE;
    stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[0] = 0x00;
    stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[1] = 0xFFFFFF;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = RK_FALSE;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 100;
    MPP_CHN_S stMppChn;
    stMppChn.enModId = RK_ID_VENC;
    stMppChn.s32DevId = 0;
    stMppChn.s32ChnId = g_venc_channel;
    ret = RK_MPI_RGN_AttachToChn(g_osd, &stMppChn, &stRgnChnAttr);
    if (ret != RK_SUCCESS) {
        RK_MPI_RGN_Destroy(g_osd);
        return ret;
    }
    g_osd_w = width;
    g_osd_h = height;
    g_osd_inited = true;
    return RK_SUCCESS;
}

int overlay_deinit(int venc_channel) {
    if (!g_osd_inited) return RK_SUCCESS;
    MPP_CHN_S stMppChn;
    stMppChn.enModId = RK_ID_VENC;
    stMppChn.s32DevId = 0;
    stMppChn.s32ChnId = venc_channel;
    RK_MPI_RGN_DetachFromChn(g_osd, &stMppChn);
    RK_MPI_RGN_Destroy(g_osd);
    g_osd_inited = false;
    g_osd_w = g_osd_h = 0;
    return RK_SUCCESS;
}

int overlay_draw_detections(int model_w, int model_h, const yolo_det_t* dets, int count, uint32_t color) {
    if (!g_osd_inited) return RK_FAILURE;
    RGN_CANVAS_INFO_S stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(stCanvasInfo));
    if (RK_MPI_RGN_GetCanvasInfo(g_osd, &stCanvasInfo) != RK_SUCCESS) return RK_FAILURE;
    uint32_t* buf32 = (uint32_t*)(uintptr_t)stCanvasInfo.u64VirAddr;
    osd_clear(buf32, stCanvasInfo.u32VirWidth, stCanvasInfo.u32VirHeight);
    for (int i = 0; i < count; ++i) {
        int x0 = dets[i].left * g_osd_w / model_w;
        int y0 = dets[i].top * g_osd_h / model_h;
        int x1 = dets[i].right * g_osd_w / model_w;
        int y1 = dets[i].bottom * g_osd_h / model_h;
        osd_draw_rect(buf32, stCanvasInfo.u32VirWidth, stCanvasInfo.u32VirHeight, x0, y0, x1, y1, 3, color);
        char label[64];
        const char* name = yolo_cls_name(dets[i].cls_id);
        int l = snprintf(label, sizeof(label), "%s %.2f", name ? name : "CLS", dets[i].conf);
        if (l > 0) {
            int tx = x0;
            int ty = y0 - 16;
            if (ty < 0) ty = y0 + 4;
            osd_draw_text(buf32, stCanvasInfo.u32VirWidth, stCanvasInfo.u32VirHeight, tx, ty, label, color, 2);
        }
    }
    RK_MPI_RGN_UpdateCanvas(g_osd);
    return RK_SUCCESS;
}

