#include <jni.h>

#include <android/asset_manager_jni.h>

#include <codecvt>
#include <locale>

#include "vr/gvr/capi/include/gvr.h"
#include "wifidiscovery_renderer.h"

extern "C" {

JNIEXPORT jlong JNICALL 
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_createRenderer(
  JNIEnv *env, jclass cls, jlong gvrContext, jobject assetMgr) {   

  WiFiDiscoveryRenderer *renderer =
    new WiFiDiscoveryRenderer(reinterpret_cast<gvr_context*>(gvrContext),
      AAssetManager_fromJava(env, assetMgr));
  return reinterpret_cast<intptr_t>(renderer);
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeOnSurfaceCreated(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->OnSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeOnDrawFrame(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->OnDrawFrame();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeProgressUpdate(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->PublishProgress();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeSetState(
    JNIEnv *env, jclass cls, jlong renderer, jint state) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->SetState(state);
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeSetScanComplete(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->SetScanComplete();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeAddHost(
    JNIEnv *env, jclass cls, jlong renderer, jstring result) {

// Convert the jstring to a wstring
  const jchar* jchars = env->GetStringChars(result, NULL);
  jsize length = env->GetStringLength(result);
  std::wstring wstr;
  wstr.assign(jchars, jchars + length);

// Convert the wstring to a string
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::string str = converter.to_bytes(wstr);

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->AddHost(str);
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeOnResume(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->OnResume();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeOnPause(
    JNIEnv *env, jclass cls, jlong renderer) {

  reinterpret_cast<WiFiDiscoveryRenderer *>(renderer)->OnPause();
}

JNIEXPORT void JNICALL
Java_com_quiller_wifidiscovery_WiFiDiscoveryActivity_nativeOnDestroy(
    JNIEnv *env, jclass cls, jlong renderer) {

  delete reinterpret_cast<WiFiDiscoveryRenderer *>(renderer);
}

}
