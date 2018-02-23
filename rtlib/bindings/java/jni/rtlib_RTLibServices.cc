#include <jni.h>
#include <bbque/bbque_exc.h>
#include "rtlib_RTLibServices.h"

jlong Java_bbque_rtlib_RTLibServices_initNative(JNIEnv *env, jobject obj, jstring java_name) {
	const char *native_name = env->GetStringUTFChars(java_name, JNI_FALSE);
	RTLIB_Services_t *rtlib;
	RTLIB_Init(native_name, &rtlib);
	jlong native_pointer;
	if (!rtlib) {
		native_pointer = 0;
		jclass exception_class = env->FindClass("bbque/rtlib/exception/RTLibInitException");
		env->ThrowNew(exception_class, "Error during initialization");
	} else {
		native_pointer = reinterpret_cast<jlong>(rtlib);		
	}
	env->ReleaseStringUTFChars(java_name, native_name);
	return native_pointer;
}
