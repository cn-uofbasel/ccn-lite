#include <stdio.h>
#include "CCNLiteInterface.h"
#include "ccnb.c"
#include "ccn-lite-ccnb2xml-java.c"

// #ifdef __APPLE__
#include "open_memstream.h"
// #endif

JNIEXPORT jstring JNICALL
Java_CCNLiteInterface_ccnbToXml(JNIEnv *env, jobject obj, jbyteArray binaryInterest)
{
    jbyte* jInterestData = (*env)->GetByteArrayElements(env, binaryInterest, NULL);
    jsize len = (*env)->GetArrayLength(env, binaryInterest);

    unsigned char interestData[8*1024], *buf = interestData;
    bzero(interestData, 8*1024);
    memcpy(interestData, jInterestData, len);


    // For now the result is written to a file (stream buffer does crash for some reason)
    remove("c_mxl.txt");
    FILE *file = fopen("c_xml.txt", "w");
    if (file == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    // Writes the ccnb interest as xml to the stream
    ccnb2xml(0, interestData, &buf, &len, NULL, file, true);

    fflush(file);
    close(file);


    // Read the file again and convert it into a jstring
    file = fopen("c_xml.txt", "r");
    char *buffer;
    long file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    buffer = malloc(file_size + 1);
    jstring java_string;
    if (buffer != NULL )
    {
        fread(buffer, file_size, 1, file);
        buffer[file_size] = '\0';
        close(file);

        java_string = (*env)->NewStringUTF(env, buffer);

        free(buffer);
        buffer = NULL;
    } else {
        fprintf(stderr, "Error when allocating buffer");
        exit(3);
    }

    if (file != NULL) fclose(file);
    // (*env)->ReleaseByteArrayElements(env, binaryInterest, jInterestData, 0);
    // TODO remove temp file
    // remove("c_mxl.txt");

    // fprintf("'%s'", java_string);
    // printf("======\n%s======\n", (*env)->GetStringUTFChars(env, java_string, 0));


    return java_string;
}

JNIEXPORT jbyteArray JNICALL
Java_CCNLiteInterface_mkBinaryContent(JNIEnv *env, jobject obj, jstring j_name, jbyteArray j_content)
{
    // Convert name from java string to c string
    const char *content_name = (*env)->GetStringUTFChars(env, j_name, 0);

    // Convert content from java byte array to c byte array
    jbyte* jbyte_content = (*env)->GetByteArrayElements(env, j_content, NULL);
    jsize content_len = (*env)->GetArrayLength(env, j_content);

    unsigned char content_data[8*1024], *buf = jbyte_content;
    memcpy(content_data, jbyte_content, content_len);

    unsigned char out[65*1024];
    unsigned char *publisher = -1;
    int i = 0, binary_content_len, publisher_len = 0;
    char *prefix[CCNL_MAX_NAME_COMP], *cp;
    char *private_key_path = 0;

    cp = strtok(content_name, "/");
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefix[i++] = cp;
        cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;


    binary_content_len = mkContent(prefix, publisher, publisher_len, private_key_path, content_data, content_len, out);
    // binary_content_len = 0;

    jbyteArray jbyte_ccnb_content = (*env)->NewByteArray(env, binary_content_len);
    jbyte *bytes = (*env)->GetByteArrayElements(env, jbyte_ccnb_content, 0);
    memcpy(bytes, out, binary_content_len);
    (*env)->SetByteArrayRegion(env, jbyte_ccnb_content, 0, binary_content_len, bytes );


    // (*env)->ReleaseStringUTFChars(env, j_name, content_name);
    // (*env)->ReleaseByteArrayElements(env, j_content, jbyte_ccnb_content, 0);

    return jbyte_ccnb_content;
}

JNIEXPORT jbyteArray JNICALL
Java_CCNLiteInterface_mkBinaryInterest(JNIEnv *env, jobject obj, jstring j_name)
{


    const char *interest_name = (*env)->GetStringUTFChars(env, j_name, 0);

    unsigned char out[8*1024], *buf = out;
    char *minSuffix = 0, *maxSuffix = 0, *scope;
    unsigned char *digest = 0, *publisher = 0;
    char *fname = 0;
    int i = 0, f, len, opt;
    int dlen = 0, plen = 0;
    char *prefix[CCNL_MAX_NAME_COMP], *cp;
    uint32_t nonce;

    // init the nonce
    time((time_t*) &nonce);

    // split the interest name
    cp = strtok(interest_name, "/");
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefix[i++] = cp;
        cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;


    // mk the ccnb interest
    len = mkInterest(prefix,
             minSuffix, maxSuffix,
             digest, dlen,
             publisher, plen,
             scope, &nonce,
             out);

    // FILE *file = fopen("c.txt", "w");
    // if (file == NULL)
    // {
    //     printf("Error opening file!\n");
    //     exit(1);
    // }
    // fwrite(out, len, 1, file);


    jbyteArray javaByteArray = (*env)->NewByteArray(env, len);
    jbyte *bytes = (*env)->GetByteArrayElements(env, javaByteArray, 0);
    memcpy(bytes, out, len);
    (*env)->SetByteArrayRegion(env, javaByteArray, 0, len, bytes );

    // (*env)->ReleaseStringUTFChars(env, j_name, interest_name);
    // (*env)->ReleaseByteArrayElements(env, javaByteArray, bytes, 0);

    return javaByteArray;
}
