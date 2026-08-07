/**
 * Node.js N-API Bindings for HiFi-GAN Vocoder
 * 
 * Provides JavaScript interface to C++ vocoder functionality
 * for Electron desktop application.
 */

#include <napi.h>
#include <vector>
#include <string>
#include "ay_pilot_engine.h"

static ay_pilot::VoiceEngine* g_engine = nullptr;

/**
 * Initialize vocoder with model path
 * 
 * Usage (JavaScript):
 * ```javascript
 * const ayPilot = require('ay-pilot-native');
 * ayPilot.initializeVocoder('models/hifigan_v1.onnx');
 * ```
 */
Napi::Value InitializeVocoder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string model_path = info[0].As<Napi::String>();
    
    if (g_engine == nullptr) {
        g_engine = new ay_pilot::VoiceEngine(48000, 512);
    }
    
    g_engine->initialize_vocoder(model_path);
    
    return env.Undefined();
}

/**
 * Check if vocoder is ready
 * 
 * Usage (JavaScript):
 * ```javascript
 * const isReady = ayPilot.isVocoderReady();
 * console.log(isReady); // true or false
 * ```
 */
Napi::Value IsVocoderReady(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (g_engine == nullptr) {
        return Napi::Boolean::New(env, false);
    }
    
    return Napi::Boolean::New(env, g_engine->is_vocoder_ready());
}

/**
 * Train AI Clone from voice sample
 * 
 * Usage (JavaScript):
 * ```javascript
 * const voiceSample = new Int16Array(48000); // 1 second at 48kHz
 * // Fill voiceSample with audio data
 * ayPilot.trainClone(voiceSample);
 * ```
 */
Napi::Value TrainClone(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsTypedArray()) {
        Napi::TypeError::New(env, "TypedArray expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::TypedArray typed_array = info[0].As<Napi::TypedArray>();
    Napi::Int16Array int16_array = typed_array.As<Napi::Int16Array>();
    
    std::vector<int16_t> sample(int16_array.Data(), 
                                int16_array.Data() + int16_array.ElementLength());
    
    if (g_engine == nullptr) {
        g_engine = new ay_pilot::VoiceEngine(48000, 512);
    }
    
    g_engine->train_clone(sample);
    
    return env.Undefined();
}

/**
 * Set voice preset
 * 
 * Usage (JavaScript):
 * ```javascript
 * // Presets: 0=Natural, 1=Female, 2=Male, 3=Deep, 4=High, 5=Robot, 6=Whisper, 7=AIClone
 * ayPilot.setPreset(7); // AI Clone
 * ```
 */
Napi::Value SetPreset(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int preset = info[0].As<Napi::Number>().Int32Value();
    
    if (g_engine == nullptr) {
        g_engine = new ay_pilot::VoiceEngine(48000, 512);
    }
    
    g_engine->set_preset(static_cast<ay_pilot::VoicePreset>(preset));
    
    return env.Undefined();
}

/**
 * Process audio frame with vocoder
 * 
 * Usage (JavaScript):
 * ```javascript
 * const input = new Int16Array(512);
 * const output = new Int16Array(512);
 * // Fill input with audio data
 * ayPilot.processFrame(input, output);
 * // output now contains transformed audio
 * ```
 */
Napi::Value ProcessFrame(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsTypedArray() || !info[1].IsTypedArray()) {
        Napi::TypeError::New(env, "Two TypedArrays expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Int16Array input_array = info[0].As<Napi::TypedArray>().As<Napi::Int16Array>();
    Napi::Int16Array output_array = info[1].As<Napi::TypedArray>().As<Napi::Int16Array>();
    
    if (g_engine == nullptr) {
        g_engine = new ay_pilot::VoiceEngine(48000, 512);
    }
    
    g_engine->process_frame(input_array.Data(), output_array.Data(), input_array.ElementLength());
    
    return env.Undefined();
}

/**
 * Get vocoder latency
 * 
 * Usage (JavaScript):
 * ```javascript
 * const latency = ayPilot.getVocoderLatency();
 * console.log(`Latency: ${latency.toFixed(2)} ms`);
 * ```
 */
Napi::Value GetVocoderLatency(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (g_engine == nullptr) {
        return Napi::Number::New(env, 0.0);
    }
    
    // Note: Requires exposing vocoder latency through VoiceEngine
    return Napi::Number::New(env, 0.0);
}

/**
 * Get vocoder model information
 * 
 * Usage (JavaScript):
 * ```javascript
 * const info = ayPilot.getVocoderInfo();
 * console.log(info); // "HiFi-GAN v1 Universal (48kHz, 80 mel bins)"
 * ```
 */
Napi::Value GetVocoderInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (g_engine == nullptr) {
        return Napi::String::New(env, "Vocoder not initialized");
    }
    
    std::string model_info = "HiFi-GAN v1 Universal (48kHz, 80 mel bins, optimized for real-time)";
    return Napi::String::New(env, model_info);
}

/**
 * Initialize N-API module
 */
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "initializeVocoder"),
                Napi::Function::New(env, InitializeVocoder));
    
    exports.Set(Napi::String::New(env, "isVocoderReady"),
                Napi::Function::New(env, IsVocoderReady));
    
    exports.Set(Napi::String::New(env, "trainClone"),
                Napi::Function::New(env, TrainClone));
    
    exports.Set(Napi::String::New(env, "setPreset"),
                Napi::Function::New(env, SetPreset));
    
    exports.Set(Napi::String::New(env, "processFrame"),
                Napi::Function::New(env, ProcessFrame));
    
    exports.Set(Napi::String::New(env, "getVocoderLatency"),
                Napi::Function::New(env, GetVocoderLatency));
    
    exports.Set(Napi::String::New(env, "getVocoderInfo"),
                Napi::Function::New(env, GetVocoderInfo));
    
    return exports;
}

NODE_API_MODULE(ay_pilot_vocoder, Init)

/**
 * Usage Example in React/Electron:
 * 
 * ```javascript
 * import { ipcRenderer } from 'electron';
 * import ayPilot from './native/ay_pilot_vocoder.node';
 * 
 * // Initialize vocoder
 * ayPilot.initializeVocoder('models/hifigan_v1.onnx');
 * 
 * // Set AI Clone preset
 * ayPilot.setPreset(7);
 * 
 * // Train from voice sample
 * const voiceSample = new Int16Array(48000);
 * ayPilot.trainClone(voiceSample);
 * 
 * // Process audio
 * const input = new Int16Array(512);
 * const output = new Int16Array(512);
 * ayPilot.processFrame(input, output);
 * 
 * // Get status
 * console.log('Vocoder ready:', ayPilot.isVocoderReady());
 * console.log('Model info:', ayPilot.getVocoderInfo());
 * ```
 */
