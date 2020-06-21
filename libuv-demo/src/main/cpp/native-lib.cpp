#include <jni.h>
#include <string>
#include <uv.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_vivo_libuv_1android_1demo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from Libuv";

    uv_cpu_info_t* cpus;
    int count;
    int err = uv_cpu_info(&cpus, &count);
    if (err != 0) {
        const char* error = uv_strerror(err);
        printf("error:  %s.\n", error);
    }

    uv_loop_t *loop = uv_default_loop();
    uv_loop_init(loop);

    printf("Now quitting.\n");
    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);

    return env->NewStringUTF(hello.c_str());
}
