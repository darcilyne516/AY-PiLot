#include <napi.h>
#include "ay_pilot_engine.h"

class VoiceEngineAddon : public Napi::ObjectWrap<VoiceEngineAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "VoiceEngine", {
            InstanceMethod("init", &VoiceEngineAddon::InitEngine),
            InstanceMethod("setPreset", &VoiceEngineAddon::SetPreset),
            InstanceMethod("process", &VoiceEngineAddon::Process)
        });
        exports.Set("VoiceEngine", func);
        return exports;
    }

    VoiceEngineAddon(const Napi::CallbackInfo& info) : Napi::ObjectWrap<VoiceEngineAddon>(info) {
        engine = new ay_pilot::VoiceEngine();
    }

    ~VoiceEngineAddon() {
        delete engine;
    }

private:
    ay_pilot::VoiceEngine* engine;

    Napi::Value InitEngine(const Napi::CallbackInfo& info) {
        int sample_rate = info[0].As<Napi::Number>().Int32Value();
        int frame_size = info[1].As<Napi::Number>().Int32Value();
        delete engine;
        engine = new ay_pilot::VoiceEngine(sample_rate, frame_size);
        return info.Env().Undefined();
    }

    Napi::Value SetPreset(const Napi::CallbackInfo& info) {
        int preset = info[0].As<Napi::Number>().Int32Value();
        engine->set_preset(static_cast<ay_pilot::VoicePreset>(preset));
        return info.Env().Undefined();
    }

    Napi::Value Process(const Napi::CallbackInfo& info) {
        Napi::Int16Array input = info[0].As<Napi::Int16Array>();
        Napi::Int16Array output = info[1].As<Napi::Int16Array>();
        int num_samples = info[2].As<Napi::Number>().Int32Value();
        
        engine->process_frame(input.Data(), output.Data(), num_samples);
        
        return info.Env().Undefined();
    }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return VoiceEngineAddon::Init(env, exports);
}

NODE_API_MODULE(ay_pilot_native, InitAll)
