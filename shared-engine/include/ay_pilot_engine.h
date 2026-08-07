#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace ay_pilot {

// Voice analysis result
struct VoiceProfile {
    float f0_mean;                          // Fundamental frequency (Hz)
    std::vector<float> formants;            // Formant frequencies (F1, F2, F3, ...)
    std::vector<float> cepstral_coeffs;     // Cepstral coefficients (13)
    std::vector<uint8_t> encrypted_data;    // AES-256 encrypted profile
};

// Voice preset enum
enum class VoicePreset {
    Natural = 0,
    Female = 1,
    Male = 2,
    Deep = 3,
    High = 4,
    Robot = 5,
    Whisper = 6,
    AIClone = 7
};

// Main voice engine class
class VoiceEngine {
public:
    VoiceEngine(int sample_rate = 48000, int frame_size = 512);
    ~VoiceEngine();
    
    // Initialize vocoder
    void initialize_vocoder(const std::string& model_path);
    bool is_vocoder_ready() const;
    
    // Voice profile management
    VoiceProfile analyze_voice(const std::vector<int16_t>& audio_sample);
    void train_clone(const std::vector<int16_t>& voice_sample);
    void load_profile(const std::string& profile_path);
    void save_profile(const std::string& profile_path);
    
    // Real-time processing
    void set_preset(VoicePreset preset);
    void process_frame(const int16_t* input, int16_t* output, int num_samples);
    
    // Utilities
    int get_sample_rate() const { return sample_rate_; }
    int get_frame_size() const { return frame_size_; }
    
private:
    int sample_rate_;
    int frame_size_;
    VoicePreset current_preset_;
    VoiceProfile current_profile_;
    bool is_clone_ready_;
    
    // Vocoder
    class HiFiGANVocoder* vocoder_;
    
    // DSP functions
    float extract_f0(const std::vector<int16_t>& audio);
    std::vector<float> extract_formants(const std::vector<int16_t>& audio);
    std::vector<float> extract_cepstral(const std::vector<int16_t>& audio);
    
    // Transformation functions
    void apply_pitch_shift(const int16_t* input, int16_t* output, 
                          int num_samples, float shift_semitones);
    void apply_formant_shift(int16_t* audio, int num_samples, float shift_factor);
    void apply_robot_effect(int16_t* audio, int num_samples);
    void apply_whisper_effect(int16_t* audio, int num_samples);
};

}  // namespace ay_pilot
