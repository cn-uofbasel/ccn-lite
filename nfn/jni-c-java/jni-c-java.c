#include <jni.h>
#include <stdio.h>


int main( int argc, const char* argv[] ) {

    /* initialization */
    JavaVM *jvm;
    JNIEnv *env;
    JavaVMInitArgs vm_args;
    int res;
    JavaVMOption options[1];
    /* Add the path to the java classes you want to use in C here */
    options[0].optionString = "Djava.class.path=.";
    vm_args.version = JNI_VERSION_1_6;
    vm_args.options = options;
    vm_args.nOptions = 1;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    /* Create the Java VM; don't forget to check the return value! */
    res = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
    if (res < 0) {
        printf("JVM could not be created");
        return 1;
    }
    /* initialization */

    jclass main_class;
    jmethodID main_method_id;

    main_class = (*env)->FindClass(env, "HelloWorld");
    if (main_class == NULL) {
     (*jvm)->DestroyJavaVM(jvm);
     printf("main_class HelloWorld not found");
     return 2;
    }

    main_method_id = (*env)->GetStaticMethodID(env, main_class, "main", "([Ljava/lang/String;)V");
    if (main_method_id == NULL) {
        printf("Could not create static method main");
        (*jvm)->DestroyJavaVM(jvm);
        return 3;
    }
    /* Finally, we can call the method */
    (*env)->CallStaticVoidMethod(env, main_class, main_method_id);

    /* Don't forget to destroy the JVM at the end */
    (*jvm)->DestroyJavaVM(jvm);


}
