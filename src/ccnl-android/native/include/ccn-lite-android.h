/*
 * @f ccn-lite-android.h
 * @b native code (library) for Android devices
 *
 * Copyright (C) 2017,  University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CCN_LITE_ANDROID
#define CCN_LITE_ANDROID

#include "ccn-lite-jni.h"

#include <dirent.h>
#include <fnmatch.h>
#include <jni.h>
#include <pthread.h>
#include <regex.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/in.h>
#include <linux/sockios.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android/looper.h>

#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-cistlv.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"
#include "ccnl-dispatch.h"


void append_to_log(char *line);

int
ccnl_open_udpdev(int port, struct sockaddr_in *si);

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf);

int
ccnl_close_socket(int s);

void ccnl_ageing(void *relay, void *aux);

void
add_udpPort(struct ccnl_relay_s *relay, int port);

void
ccnl_relay_config(struct ccnl_relay_s *relay, int httpport, char *uxpath,
                  int max_cache_entries, char *crypto_face_path);

void
ccnl_populate_cache(struct ccnl_relay_s *ccnl, char *path);

int
ccnl_android_timer_io(int fd, int events, void *data);

int
ccnl_android_udp_io(int fd, int events, void *data);

int
ccnl_android_http_io(int fd, int events, void *data);

int
ccnl_android_http_accept(int fd, int events, void *data);

void*
timer();

char*
ccnl_android_init();

char*
ccnl_android_getTransport();
#endif //CCN_LITE_ANDROID
