#include <stdio.h>
#include "HelloWorld.h"

JNIEXPORT jstring JNICALL
Java_HelloWorld_print(JNIEnv *env, jobject obj, jstring str)
{
    char buf[128];
    const char *c_str = (*env)->GetStringUTFChars(env, str, 0);
    printf("%s printed in C program!", c_str);
    (*env)->ReleaseStringUTFChars(env, str, c_str);
    // scanf("%s", buf);
    return (*env)->NewStringUTF(env, "c text");
}
