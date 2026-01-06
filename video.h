#ifndef VIDEO_DAEMON_VIDEO_H
#define VIDEO_DAEMON_VIDEO_H

#include <rk_common.h>

int video_init();
void video_shutdown();
void *run_detect_format(void *arg);
void video_start_streaming();
void video_stop_streaming();

void video_set_quality_factor(float factor);
void video_set_encodec_type(RK_CODEC_ID_E type);
void video_set_yolo_enable(int enable);

int udp_socket_init(int port);
int udp_socket_send(const u_int8_t* frame, ssize_t len);

#endif //VIDEO_DAEMON_VIDEO_H
