#include <jni.h>
#include <bbque/bbque_exc.h>

#ifndef _Included_bbque_rtlib_RTLibServices
#define _Included_bbque_rtlib_RTLibServices
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     bbque_rtlib_RTLibServices
 * Method:    initNative
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_bbque_rtlib_RTLibServices_initNative(JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
