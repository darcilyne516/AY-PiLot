#include <jni.h>
#include <string>
#include "ay_pilot_engine.h"

static ay_pilot::VoiceEngine* engine = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_init(JNIEnv* env, jobject thiz, jint sample_rate, jint frame_size) {
    if (engine) delete engine;
    engine = new ay_pilot::VoiceEngine(sample_rate, frame_size);
}

extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_setPreset(JNIEnv* env, jobject thiz, jint preset_index) {
    if (engine) {
        engine->set_preset(static_cast<ay_pilot::VoicePreset>(preset_index));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_process(JNIEnv* env, jobject thiz, jshortArray input, jshortArray output, jint num_samples) {
    if (engine) {
        jshort* in_ptr = env->GetShortArrayElements(input, nullptr);
        jshort* out_ptr = env->GetShortArrayElements(output, nullptr);
        
        engine->process_frame(reinterpret_cast<const int16_t*>(in_ptr), reinterpret_cast<int16_t*>(out_ptr), num_samples);
        
        env->ReleaseShortArrayElements(input, in_ptr, JNI_ABORT);
        env->ReleaseShortArrayElements(output, out_ptr, 0);
    }
}
