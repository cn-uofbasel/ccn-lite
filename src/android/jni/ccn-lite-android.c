// src/android/jni/ccn-lite-android.c

#include <string.h>
#include <jni.h>

#include "../../ccn-lite-android.c"

jstring
Java_ch_unibas_ccn_1lite_1android_CcnLiteAndroid_stringFromJNI(JNIEnv* env,
                                                               jobject thiz)
{
    char *hello = ccnl_android_init();

    return (*env)->NewStringUTF(env, hello);
}
