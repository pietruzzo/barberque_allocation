#include <cstdarg>
#include <bbque/bbque_exc.h>
#include "rtlib_BBQueEXC.h"

jlong Java_bbque_rtlib_BBQueEXC_initNative(JNIEnv *env, jobject obj, jstring javaName, jstring javaRecipe) {
	const char *nativeName = env->GetStringUTFChars(javaName, JNI_FALSE);
	const char *nativeRecipe = env->GetStringUTFChars(javaRecipe, JNI_FALSE);
	RTLIB_Services_t *rtlib;
	RTLIB_Init(nativeName, &rtlib);
	jlong native_pointer;
	if (!rtlib) {
		native_pointer = bbque_rtlib_BBQueEXC_ERROR_LIB_INIT;
	} else {
		JNIBbqueEXC *jni_exc = new JNIBbqueEXC(nativeName, nativeRecipe, rtlib, env, obj);
		if (!jni_exc) {
			native_pointer = bbque_rtlib_BBQueEXC_ERROR_EXECUTION_CONTEXT_REGISTRATION;
		} else {
			native_pointer = reinterpret_cast<jlong>(jni_exc);
		}
	}
	env->ReleaseStringUTFChars(javaName, nativeName);
	env->ReleaseStringUTFChars(javaRecipe, nativeRecipe);
	return native_pointer;
}

void Java_bbque_rtlib_BBQueEXC_disposeNative(JNIEnv *env, jobject obj, jlong native_pointer) {
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	delete jni_exc;
}

jboolean Java_bbque_rtlib_BBQueEXC_isRegistered(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->isRegistered();
}

void Java_bbque_rtlib_BBQueEXC_start(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->Start();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_waitCompletion(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->WaitCompletion();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_terminate(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->Terminate();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_enable(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->Enable();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_disable(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->Disable();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

/*
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setAWMConstraints(JNIEnv *, jobject, jobjectArray) {
	// TODO: implement this function
}*/

void Java_bbque_rtlib_BBQueEXC_clearAWMConstraints(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->ClearAWMConstraints();
	throwRTLibExceptionIfNecessary(env, obj, code);
}

/*
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setGoalGap(JNIEnv *, jobject, jint) {
	// TODO: implement this function
}*/

jstring Java_bbque_rtlib_BBQueEXC_getUniqueID_1String(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return env->NewStringUTF(jni_exc->GetUniqueID_String());
}

/*
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2(JNIEnv *, jobject, jobject) {
	// TODO: implement this function
}*/

/*
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_BBQueEXC_getAffinityMask(JNIEnv *, jobject, jintArray) {
	// TODO: implement this function
}*/

/*
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2_3I(JNIEnv *, jobject, jobject, jintArray) {
	// TODO: implement this function
}*/

void Java_bbque_rtlib_BBQueEXC_setCPS(JNIEnv *env, jobject obj, jfloat java_cps) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->SetCPS(java_cps);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_setJPSGoal(JNIEnv *env, jobject obj, jfloat java_jps_min, jfloat java_jps_max, jint java_jpc) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->SetJPSGoal(java_jps_min, java_jps_max, java_jpc);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_updateJPC(JNIEnv *env, jobject obj, jint java_jpc) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->UpdateJPC(java_jpc);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_setCPSGoal__FF(JNIEnv *env, jobject obj, jfloat java_cps_min, jfloat java_cps_max) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->SetCPSGoal(java_cps_min, java_cps_max);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_setCPSGoal__F(JNIEnv *env, jobject obj, jfloat java_cps) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->SetCPSGoal(java_cps);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

void Java_bbque_rtlib_BBQueEXC_setMinimumCycleTimeUs(JNIEnv *env, jobject obj, jint java_min_cycle_time_us) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	RTLIB_ExitCode_t code = jni_exc->SetMinimumCycleTimeUs(java_min_cycle_time_us);
	throwRTLibExceptionIfNecessary(env, obj, code);
}

jfloat Java_bbque_rtlib_BBQueEXC_getCPS(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->GetCPS();
}

jfloat Java_bbque_rtlib_BBQueEXC_getJPS(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->GetJPS();
}

jint Java_bbque_rtlib_BBQueEXC_getCTimeUs(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->GetCTimeUs();
}

jint Java_bbque_rtlib_BBQueEXC_getCTimeMs(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->GetCTimeMs();
}

jint Java_bbque_rtlib_BBQueEXC_cycles(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->Cycles();
}

/*
JNIEXPORT jobject JNICALL Java_bbque_rtlib_BBQueEXC_workingModeParams(JNIEnv *env, jobject obj) {
	// TODO: implement this function
}*/

jboolean Java_bbque_rtlib_BBQueEXC_done(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->Done();
}

jint Java_bbque_rtlib_BBQueEXC_currentAWM(JNIEnv *env, jobject obj) {
	jlong native_pointer = getEXCNativePointer(env, obj);
	JNIBbqueEXC *jni_exc = reinterpret_cast<JNIBbqueEXC *>(native_pointer);
	return jni_exc->CurrentAWM();
}

/*
JNIEXPORT jobject JNICALL Java_bbque_rtlib_BBQueEXC_configuration(JNIEnv *env, jobject obj) {
	// TODO: implement this function
}*/

RTLIB_ExitCode_t JNIBbqueEXC::onGenericIntCallback(const char *method, const char *signature, ...) {
	va_list arguments;
	va_start(arguments, signature);
	JNIEnv *env;
	jvm->AttachCurrentThread((void **)&env, NULL);
	jmethodID callback_method = env->GetMethodID(callback_class, method, signature);
	jint exit_code = env->CallIntMethodV(callback_obj, callback_method, arguments);
	va_end(arguments);
	jvm->DetachCurrentThread();
	return RTLIB_OK; // TODO: find a way to convert exit codes from java to cpp
}

jlong getEXCNativePointer(JNIEnv *env, jobject obj) {
	jclass java_class = env->GetObjectClass(obj);
	jfieldID native_pointer_id = env->GetFieldID(java_class, "mNativePointer", "J");
	return env->GetLongField(obj, native_pointer_id);
}

void throwRTLibExceptionIfNecessary(JNIEnv *env, jobject obj, RTLIB_ExitCode_t exit_code) {
	if (exit_code != RTLIB_OK) {
        	// TODO: throw an RTLibException with the given exit_code (cpp to java)
	}
}
