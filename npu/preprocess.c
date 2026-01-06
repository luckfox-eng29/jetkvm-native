#include "preprocess.h"

static inline int clamp8_local(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void yuyv_to_rgb(const uint8_t* yuyv, uint8_t* rgb, int width, int height) {
    const int stride = width * 2;
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

