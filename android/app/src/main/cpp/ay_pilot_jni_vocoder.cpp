#include <jni.h>
#include <string>
#include <vector>
#include "ay_pilot_engine.h"

/**
 * Android JNI Bindings for HiFi-GAN Vocoder
 * 
 * Provides Java/Kotlin interface to C++ vocoder functionality:
 * - Initialize vocoder with model path
 * - Train AI Clone from voice sample
 * - Synthesize audio from voice profile
 * - Query vocoder status and latency
 */

static ay_pilot::VoiceEngine* g_engine = nullptr;

// Initialize native engine
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_initializeNative(
    JNIEnv* env, jobject /* this */, jint sample_rate, jint frame_size) {
    
    if (g_engine == nullptr) {
        g_engine = new ay_pilot::VoiceEngine(sample_rate, frame_size);
    }
}

// Initialize HiFi-GAN vocoder
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_initializeVocoder(
    JNIEnv* env, jobject /* this */, jstring model_path) {
    
    if (g_engine == nullptr) return;
    
    const char* path = env->GetStringUTFChars(model_path, nullptr);
    g_engine->initialize_vocoder(std::string(path));
    env->ReleaseStringUTFChars(model_path, path);
}

// Check if vocoder is ready
extern "C" JNIEXPORT jboolean JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_isVocoderReady(
    JNIEnv* env, jobject /* this */) {
    
    if (g_engine == nullptr) return false;
    return g_engine->is_vocoder_ready();
}

// Train AI Clone from voice sample
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_trainClone(
    JNIEnv* env, jobject /* this */, jshortArray sample_array) {
    
    if (g_engine == nullptr) return;
    
    jshort* sample_data = env->GetShortArrayElements(sample_array, nullptr);
    jsize sample_length = env->GetArrayLength(sample_array);
    
    std::vector<int16_t> sample(sample_data, sample_data + sample_length);
    g_engine->train_clone(sample);
    
    env->ReleaseShortArrayElements(sample_array, sample_data, JNI_ABORT);
}

// Set voice preset for AI Clone
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_setPreset(
    JNIEnv* env, jobject /* this */, jint preset) {
    
    if (g_engine == nullptr) return;
    g_engine->set_preset(static_cast<ay_pilot::VoicePreset>(preset));
}

// Process audio frame with vocoder
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_processFrameVocoder(
    JNIEnv* env, jobject /* this */, 
    jshortArray input_array, jshortArray output_array) {
    
    if (g_engine == nullptr) return;
    
    jshort* input_data = env->GetShortArrayElements(input_array, nullptr);
    jshort* output_data = env->GetShortArrayElements(output_array, nullptr);
    jsize frame_size = env->GetArrayLength(input_array);
    
    g_engine->process_frame(input_data, output_data, frame_size);
    
    env->ReleaseShortArrayElements(input_array, input_data, JNI_ABORT);
    env->ReleaseShortArrayElements(output_array, output_data, 0);
}

// Get vocoder latency
extern "C" JNIEXPORT jfloat JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_getVocoderLatency(
    JNIEnv* env, jobject /* this */) {
    
    if (g_engine == nullptr) return 0.0f;
    
    // Note: This requires exposing vocoder latency through VoiceEngine
    // For now, return a placeholder
    return 0.0f;
}

// Get vocoder model info
extern "C" JNIEXPORT jstring JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_getVocoderInfo(
    JNIEnv* env, jobject /* this */) {
    
    if (g_engine == nullptr) {
        return env->NewStringUTF("Vocoder not initialized");
    }
    
    // Return model information
    std::string info = "HiFi-GAN v1 Universal (48kHz, 80 mel bins)";
    return env->NewStringUTF(info.c_str());
}

// Cleanup
extern "C" JNIEXPORT void JNICALL
Java_com_aypilot_nativeapp_VoiceEngine_cleanup(
    JNIEnv* env, jobject /* this */) {
    
    if (g_engine != nullptr) {
        delete g_engine;
        g_engine = nullptr;
    }
}
