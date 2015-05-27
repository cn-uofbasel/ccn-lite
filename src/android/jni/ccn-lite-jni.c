// src/android/jni/ccn-lite-jni.c

#include <string.h>
#include <jni.h>

int jni_bleSend(unsigned char *data, int len);

#include "../../ccn-lite-android.c"

static JavaVM *jvm;
static jclass ccnLiteClass;
static jobject ccnLiteObject;


JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayInit(JNIEnv* env,
                                                            jobject thiz)
{
    char *hello;

    (*env)->GetJavaVM(env, &jvm);

    if (ccnLiteClass == NULL) {
        jclass localRefCls = (*env)->FindClass(env,
                             "ch/unibas/ccn_lite_android/CcnLiteAndroid"); 
        if (localRefCls != NULL)
            ccnLiteClass = (*env)->NewGlobalRef(env, localRefCls);
        (*env)->DeleteLocalRef(env, localRefCls);
    }
    if (ccnLiteObject == NULL)
        ccnLiteObject = (*env)->NewGlobalRef(env, thiz);

    hello = ccnl_android_init();
    return (*env)->NewStringUTF(env, hello);
}

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayGetTransport(JNIEnv* env,
                                                                   jobject thiz)
{
    return (*env)->NewStringUTF(env, ccnl_android_getTransport());
}

char*
lvl2str(int level)
{
    switch (level) {
        case FATAL:     return "loglevel = fatal";
        case ERROR:     return "loglevel &le; error";
        case WARNING:   return "loglevel &le; warning";
        case INFO:      return "loglevel &le; info";
        case DEBUG:     return "loglevel &le; debug";
        case VERBOSE:   return "loglevel &le; verbose";
        case TRACE:     return "loglevel &le; trace";
        default:        return "loglevel = ?";
    }
}

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayPlus(JNIEnv* env,
                                                           jobject thiz)
{
    if (debug_level < TRACE)
        debug_level++;

    return (*env)->NewStringUTF(env, lvl2str(debug_level));
}

JNIEXPORT jstring JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayMinus(JNIEnv* env,
                                                            jobject thiz)
{
    if (debug_level > 0)
        debug_level--;

    return (*env)->NewStringUTF(env, lvl2str(debug_level));
}

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayDump(JNIEnv* env,
                                                           jobject thiz)
{
    debug_memdump();
}

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayTimer(JNIEnv* env,
                                                           jobject thiz)
{
    DEBUGMSG(DEBUG, "relayTimer");
    ccnl_run_events();
}

void
add_route(char *pfx, struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd;
    char buf[100];

    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    if (!fwd)
        return;

    DEBUGMSG(INFO, "adding a route for prefix %s\n", pfx);
    
    strcpy(buf, pfx);
    fwd->prefix = ccnl_URItoPrefix(buf, CCNL_SUITE_NDNTLV, NULL, NULL);
    fwd->face = face;
    fwd->suite = CCNL_SUITE_NDNTLV;
    fwd->next = theRelay.fib;
    theRelay.fib = fwd;
}

JNIEXPORT void JNICALL
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_relayRX(JNIEnv* env,
                                                         jobject thiz,
                                                         jbyteArray addr,
                                                         jbyteArray data)
{
    int len;
    unsigned char buf[1024];
    sockunion su;

    DEBUGMSG(DEBUG, "relayRX: %d bytes\n", len);

    memset(&su, 0, sizeof(su));
    su.eth.sll_family = AF_PACKET;
    (*env)->GetByteArrayRegion(env, addr, 0, ETH_ALEN,
                               (signed char*) &su.eth.sll_addr);

    len = (*env)->GetArrayLength(env, data);
    if (len > sizeof(buf))
        len = sizeof(buf);
    (*env)->GetByteArrayRegion(env, data, 0, len, (signed char*) buf);

    ccnl_core_RX(&theRelay, 1, buf, len, (struct sockaddr*) &su, sizeof(su));

    // hack: when the first packet from the BT LE device is received,
    // (and the FIB is empty), install two forwarding entries
    if (theRelay.faces && (!theRelay.fib || theRelay.fib->tap)) {
        theRelay.faces->flags |= CCNL_FACE_FLAGS_STATIC;
        add_route("/TinC", theRelay.faces);
        add_route("/TinF", theRelay.faces);
        return;
    }
}

void jni_append_to_log(char *line)
{
    JNIEnv *env;
    int len = strlen(line);

    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';

    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_4);
    jmethodID method = (*env)->GetMethodID(env, ccnLiteClass,
                                           "appendToLog",
                                           "(Ljava/lang/String;)V");
    (*env)->CallVoidMethod(env, ccnLiteObject, method,
                           (*env)->NewStringUTF(env, line));
}

int jni_bleSend(unsigned char *data, int len)
{
    JNIEnv *env;
    jmethodID method;
    jbyteArray a;

    if (len > 20) {
        DEBUGMSG(WARNING, "  bleSend: trimming from %d to 20 bytes!\n", len);
        len = 20;
    }

    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_4);

    a = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, a, 0, len, data);

    method = (*env)->GetMethodID(env, ccnLiteClass,
                                 "bleSend", "([B)V");
    (*env)->CallVoidMethod(env, ccnLiteObject, method, a);

    return len;
}
