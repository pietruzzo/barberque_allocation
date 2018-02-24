#include <jni.h>
#include <bbque/bbque_exc.h>

#ifndef _Included_bbque_rtlib_model_BBQueEXC
#define _Included_bbque_rtlib_model_BBQueEXC

using namespace bbque::rtlib;

class JNIBbqueEXC : public BbqueEXC {

public:

	JNIBbqueEXC(std::string const & name, std::string const & recipe, RTLIB_Services_t *rtlib, JNIEnv *env, jobject obj)
												: BbqueEXC(name, recipe, rtlib) {
		env->GetJavaVM(&jvm);
		callback_obj = env->NewGlobalRef(obj);
		jclass obj_class = env->GetObjectClass(obj);
		callback_class = reinterpret_cast<jclass>(env->NewGlobalRef(obj_class));
	}

	virtual ~JNIBbqueEXC() {
		delete jvm;
	}

private:

	JavaVM *jvm;
	jobject callback_obj;
	jclass callback_class;

	RTLIB_ExitCode_t onSetup() override {
		return onGenericIntCallback("onSetupCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onConfigure(int8_t awm_id) override {
		return onGenericIntCallback("onConfigureCallback", "(I)Lbbque/rtlib/enumeration/RTLibExitCode;", (jint) awm_id);
	}

	RTLIB_ExitCode_t onSuspend() override {
		return onGenericIntCallback("onSuspendCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onResume() override {
		return onGenericIntCallback("onResumeCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onRun() override {
		return onGenericIntCallback("onRunCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onMonitor() override {
		return onGenericIntCallback("onMonitorCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onRelease() override {
		return onGenericIntCallback("onReleaseCallback", "()Lbbque/rtlib/enumeration/RTLibExitCode;");
	}

	RTLIB_ExitCode_t onGenericIntCallback(const char *method, const char *signature, ...);
};

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    initNative
 * Signature: (Ljava/lang/String;Ljava/lang/String;Lbbque/rtlib/model/RTLibServices;)J
 */
JNIEXPORT jlong JNICALL Java_bbque_rtlib_model_BBQueEXC_initNative
  (JNIEnv *, jobject, jstring, jstring, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    isRegistered
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_bbque_rtlib_model_BBQueEXC_isRegistered
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_start
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    waitCompletion
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_waitCompletion
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    terminate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_terminate
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    enable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_enable
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    disable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_disable
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setAWMConstraints
 * Signature: ([Lbbque/rtlib/model/RTLibConstraint;)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setAWMConstraints
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    clearAWMConstraints
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_clearAWMConstraints
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setGoalGap
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setGoalGap
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getUniqueID_String
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_bbque_rtlib_model_BBQueEXC_getUniqueID_1String
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getUniqueID
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_getUniqueID
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getAssignedResources
 * Signature: (Lbbque/rtlib/enumeration/RTLibResourceType;)I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2
  (JNIEnv *, jobject, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getAffinityMask
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_model_BBQueEXC_getAffinityMask
  (JNIEnv *, jobject, jintArray);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getAssignedResources
 * Signature: (Lbbque/rtlib/enumeration/RTLibResourceType;[I)[I
 */
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_model_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2_3I
  (JNIEnv *, jobject, jobject, jintArray);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setCPS
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setCPS
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setJPSGoal
 * Signature: (FFI)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setJPSGoal
  (JNIEnv *, jobject, jfloat, jfloat, jint);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    updateJPC
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_updateJPC
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setCPSGoal
 * Signature: (FF)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setCPSGoal__FF
  (JNIEnv *, jobject, jfloat, jfloat);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setCPSGoal
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setCPSGoal__F
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    setMinimumCycleTimeUs
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_model_BBQueEXC_setMinimumCycleTimeUs
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getCPS
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_bbque_rtlib_model_BBQueEXC_getCPS
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getJPS
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_bbque_rtlib_model_BBQueEXC_getJPS
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getCTimeUs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_getCTimeUs
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    getCTimeMs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_getCTimeMs
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    cycles
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_cycles
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    workingModeParams
 * Signature: ()Lbbque/rtlib/model/RTLibWorkingModeParams;
 */
JNIEXPORT jobject JNICALL Java_bbque_rtlib_model_BBQueEXC_workingModeParams
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    done
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_bbque_rtlib_model_BBQueEXC_done
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    currentAWM
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_model_BBQueEXC_currentAWM
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_model_BBQueEXC
 * Method:    configuration
 * Signature: ()Lbbque/rtlib/model/RTLibConfig;
 */
JNIEXPORT jobject JNICALL Java_bbque_rtlib_model_BBQueEXC_configuration
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
