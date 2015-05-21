// src/android/jni/ccn-lite-android.c

#include <string.h>
#include <jni.h>

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
//
    DEBUGMSG(DEBUG, "relayTimer");
    ccnl_run_events();
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
