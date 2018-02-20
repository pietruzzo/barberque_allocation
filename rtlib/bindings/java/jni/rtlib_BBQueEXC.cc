#include <bbque/bbque_exc.h>
#include "rtlib_BBQueEXC.h"
#include <iostream>

jlong Java_bbque_rtlib_BBQueEXC_initNative(JNIEnv *env, jobject obj, jstring javaName, jstring javaRecipe) {
	const char *nativeName = env->GetStringUTFChars(javaName, JNI_FALSE);
	const char *nativeRecipe = env->GetStringUTFChars(javaRecipe, JNI_FALSE);
	std::cout << "[JNI] initializing RTLIB..." << std::endl;
	RTLIB_Services_t *rtlib;
	RTLIB_Init(nativeName, &rtlib);
	jlong native_pointer;
	if (!rtlib) {
		std::cout << "[JNI] initializing RTLIB...FAILED!!!" << std::endl;
		native_pointer = bbque_rtlib_BBQueEXC_ERROR_LIB_INIT;
	} else {
		std::cout << "[JNI] initializing RTLIB...OK" << std::endl;
		std::cout << "[JNI] initializing APP..." << std::endl;
		JNIBbqueEXC *jni_exc = new JNIBbqueEXC(nativeName, nativeRecipe, rtlib);
		if (!jni_exc) {
			std::cout << "[JNI] initializing APP...FAILED!!!" << std::endl;
			native_pointer = bbque_rtlib_BBQueEXC_ERROR_EXECUTION_CONTEXT_REGISTRATION;
		} else {
			std::cout << "[JNI] initializing APP...OK" << std::endl;
			native_pointer = reinterpret_cast<jlong>(jni_exc);
		}
	}
	std::cout << "[JNI] releasing memory..." << std::endl;
	env->ReleaseStringUTFChars(javaName, nativeName);
	env->ReleaseStringUTFChars(javaRecipe, nativeRecipe);
	std::cout << "[JNI] returning pointer to JAVA" << std::endl;
	return native_pointer;
}

void Java_bbque_rtlib_BBQueEXC_start(JNIEnv *env, jobject obj) {
	std::cout << "[JNI] Obtaining native pointer from JAVA..." << std::endl;
	jclass java_class = env->GetObjectClass(obj);
	jfieldID native_pointer_id = env->GetFieldID(java_class, "mNativePointer", "J");
	jlong native_pointer = env->GetLongField(obj, native_pointer_id);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	std::cout << "[JNI] the execution is starting..." << std::endl;
	jni_exc->Start();
	// TODO: check response and throw an exception if necessary
}
