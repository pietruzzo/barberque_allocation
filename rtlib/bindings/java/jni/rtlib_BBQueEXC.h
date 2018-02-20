#include <jni.h>
#include <bbque/bbque_exc.h>
#include <iostream>

#ifndef _Included_bbque_rtlib_BBQueEXC
#define _Included_bbque_rtlib_BBQueEXC

using namespace bbque::rtlib;

class JNIBbqueEXC : public BbqueEXC {

public:

	JNIBbqueEXC(std::string const & name, std::string const & recipe, RTLIB_Services_t *rtlib) : BbqueEXC(name, recipe, rtlib) {}

	virtual ~JNIBbqueEXC() {}

private:

};

#ifdef __cplusplus
extern "C" {
#endif

#define bbque_rtlib_BBQueEXC_ERROR_LIB_INIT -1L
#define bbque_rtlib_BBQueEXC_ERROR_EXECUTION_CONTEXT_REGISTRATION -2L

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    initNative
 * Signature: (Ljava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_bbque_rtlib_BBQueEXC_initNative(JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    disposeNative
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_disposeNative
  (JNIEnv *, jobject, jlong);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    isRegistered
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_bbque_rtlib_BBQueEXC_isRegistered
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_start(JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    waitCompletion
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_waitCompletion
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    terminate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_terminate
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    enable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_enable
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    disable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_disable
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setAWMConstraints
 * Signature: ([Lbbque/rtlib/RTLibConstraint;)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setAWMConstraints
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    clearAWMConstraints
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_clearAWMConstraints
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setGoalGap
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setGoalGap
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getUniqueID_String
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_bbque_rtlib_BBQueEXC_getUniqueID_1String
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getAssignedResources
 * Signature: (Lbbque/rtlib/enumeration/RTLibResourceType;)I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2
  (JNIEnv *, jobject, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getAffinityMask
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_BBQueEXC_getAffinityMask
  (JNIEnv *, jobject, jintArray);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getAssignedResources
 * Signature: (Lbbque/rtlib/enumeration/RTLibResourceType;[I)[I
 */
JNIEXPORT jintArray JNICALL Java_bbque_rtlib_BBQueEXC_getAssignedResources__Lbbque_rtlib_enumeration_RTLibResourceType_2_3I
  (JNIEnv *, jobject, jobject, jintArray);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setCPS
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setCPS
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setJPSGoal
 * Signature: (FFI)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setJPSGoal
  (JNIEnv *, jobject, jfloat, jfloat, jint);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    updateJPC
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_updateJPC
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setCPSGoal
 * Signature: (FF)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setCPSGoal__FF
  (JNIEnv *, jobject, jfloat, jfloat);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setCPSGoal
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setCPSGoal__F
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    setMinimumCycleTimeUs
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_bbque_rtlib_BBQueEXC_setMinimumCycleTimeUs
  (JNIEnv *, jobject, jint);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getCPS
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_bbque_rtlib_BBQueEXC_getCPS
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getJPS
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_bbque_rtlib_BBQueEXC_getJPS
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getCTimeUs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_getCTimeUs
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    getCTimeMs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_getCTimeMs
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    cycles
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_cycles
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    workingModeParams
 * Signature: ()Lbbque/rtlib/RTLibWorkingModeParams;
 */
JNIEXPORT jobject JNICALL Java_bbque_rtlib_BBQueEXC_workingModeParams
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    done
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_bbque_rtlib_BBQueEXC_done
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    currentAWM
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_bbque_rtlib_BBQueEXC_currentAWM
  (JNIEnv *, jobject);

/*
 * Class:     bbque_rtlib_BBQueEXC
 * Method:    configuration
 * Signature: ()Lbbque/rtlib/RTLibConf;
 */
JNIEXPORT jobject JNICALL Java_bbque_rtlib_BBQueEXC_configuration
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
