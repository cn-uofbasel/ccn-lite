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

void jni_append_to_log(char *line)
{
    JNIEnv *env;

    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_4);
    jmethodID method = (*env)->GetMethodID(env, ccnLiteClass,
                                           "appendToLog",
                                           "(Ljava/lang/String;)V");
    (*env)->CallVoidMethod(env, ccnLiteObject, method,
                           (*env)->NewStringUTF(env, line));
}
