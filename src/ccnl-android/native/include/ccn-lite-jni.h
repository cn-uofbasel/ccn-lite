#include <string.h>
#include <jni.h>


#ifndef CCN_LITE_JNI
#define CCN_LITE_JNI
static JavaVM *jvm;
static jclass ccnLiteClass;
static jobject ccnLiteObject;

#include "ccnl-core.h"

int jni_bleSend(unsigned char *data, int len);

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayInit(JNIEnv* env,
                                                           jobject thiz);

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayGetTransport(JNIEnv* env,
                                                                 jobject thiz);

char*
lvl2str(int level);

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayPlus(JNIEnv* env,
                                                         jobject thiz);

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayMinus(JNIEnv* env,
                                                          jobject thiz);

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayDump(JNIEnv* env,
                                                            jobject thiz);

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayTimer(JNIEnv* env,
                                                            jobject thiz);

void
add_route(char *pfx, struct ccnl_face_s *face, int suite, int mtu);

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayRX(JNIEnv* env,
                                                            jobject thiz,
                                                            jbyteArray addr,
                                                            jbyteArray data);

void jni_append_to_log(char *line);

int jni_bleSend(unsigned char *data, int len);


#endif //CCN_LITE_JNI




