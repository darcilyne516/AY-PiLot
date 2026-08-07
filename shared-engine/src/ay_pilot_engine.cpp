#include "ay_pilot_engine.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <vector>

namespace ay_pilot {

// Helper functions for signal processing
void apply_hann_window(std::vector<float>& frame) {
    int N = frame.size();
    for (int n = 0; n < N; ++n) {
        frame[n] *= 0.5f * (1.0f - cosf(2.0f * M_PI * n / (N - 1)));
    }
}

float estimate_f0_autocorrelation(const std::vector<float>& frame, int sample_rate) {
    int N = frame.size();
    int min_lag = sample_rate / 400; // 400 Hz
    int max_lag = sample_rate / 50;  // 50 Hz
    
    float max_corr = -1.0f;
    int best_lag = -1;

    for (int lag = min_lag; lag <= max_lag; ++lag) {
        float corr = 0;
        for (int i = 0; i < N - lag; ++i) {
            corr += frame[i] * frame[i + lag];
        }
        if (corr > max_corr) {
            max_corr = corr;
            best_lag = lag;
        }
    }

    if (best_lag > 0) {
        return (float)sample_rate / best_lag;
    }
    return 0.0f;
}

// Levinson-Durbin algorithm for LPC coefficients
std::vector<float> solve_levinson_durbin(const std::vector<float>& r, int order) {
    std::vector<float> a(order + 1, 0.0f);
    std::vector<float> e(order + 1, 0.0f);
    a[0] = 1.0f;
    e[0] = r[0];

    for (int k = 1; k <= order; ++k) {
        float lambda = 0;
        for (int j = 0; j < k; ++j) {
            lambda -= a[j] * r[k - j];
        }
        lambda /= e[k - 1];

        std::vector<float> a_next = a;
        for (int j = 1; j < k; ++j) {
            a_next[j] = a[j] + lambda * a[k - j];
        }
        a_next[k] = lambda;
        a = a_next;
        e[k] = (1.0f - lambda * lambda) * e[k - 1];
    }
    return a;
}

VoiceEngine::VoiceEngine(int sample_rate, int frame_size)
    : sample_rate_(sample_rate),
      frame_size_(frame_size),
      current_preset_(VoicePreset::Natural),
      pitch_shift_(0.0f),
      formant_shift_(1.0f),
      noise_suppression_enabled_(false),
      echo_cancellation_enabled_(false) {
    input_buffer_.resize(frame_size_);
    output_buffer_.resize(frame_size_);
}

VoiceEngine::~VoiceEngine() {}

void VoiceEngine::train_clone(const std::vector<int16_t>& sample) {
    // 1. Analyze voice sample to extract profile
    current_profile_ = analyze_voice(sample);
    
    // 2. Extract mel-spectrogram features for vocoder
    // This will be used during synthesis
    
    // 3. Load profile into vocoder
    if (vocoder_) {
        is_clone_ready_ = true;
    }
}

void VoiceEngine::load_profile(const VoiceProfile& profile) {
    // Load voice profile for AI Clone synthesis
    current_profile_ = profile;
    
    // Prepare vocoder for synthesis
    if (vocoder_) {
        is_clone_ready_ = true;
    }
}

void VoiceEngine::initialize_vocoder(const std::string& model_path) {
    // Initialize HiFi-GAN vocoder with pre-trained model
    vocoder_ = std::make_unique<HiFiGANVocoder>(model_path, sample_rate_);
}

bool VoiceEngine::is_vocoder_ready() const {
    return vocoder_ && vocoder_->is_ready();
}

std::vector<uint8_t> VoiceEngine::encrypt_profile(const VoiceProfile& profile, const std::string& key) {
    // Mock AES-256 encryption
    std::string mock_data = profile.name + "|f0:" + std::to_string(profile.f0_mean);
    return std::vector<uint8_t>(mock_data.begin(), mock_data.end());
}

VoiceProfile VoiceEngine::decrypt_profile(const std::vector<uint8_t>& data, const std::string& key) {
    VoiceProfile profile;
    std::string mock_data(data.begin(), data.end());
    profile.name = "Decrypted Profile";
    profile.f0_mean = 120.0f;
    return profile;
}

VoiceProfile VoiceEngine::analyze_voice(const std::vector<int16_t>& audio_samples) {
    VoiceProfile profile;
    profile.name = "Analyzed Profile";
    
    // 1. Framing and Pre-emphasis
    std::vector<float> float_samples(audio_samples.begin(), audio_samples.end());
    // Pre-emphasis filter: y[n] = x[n] - 0.97 * x[n-1]
    for (size_t i = float_samples.size() - 1; i > 0; --i) {
        float_samples[i] -= 0.97f * float_samples[i - 1];
    }

    // 2. F0 Extraction (Simplified: average over frames)
    int num_frames = float_samples.size() / frame_size_;
    float total_f0 = 0;
    int voiced_frames = 0;

    for (int f = 0; f < num_frames; ++f) {
        std::vector<float> frame(float_samples.begin() + f * frame_size_, float_samples.begin() + (f + 1) * frame_size_);
        apply_hann_window(frame);
        float f0 = estimate_f0_autocorrelation(frame, sample_rate_);
        if (f0 > 50.0f && f0 < 400.0f) {
            total_f0 += f0;
            voiced_frames++;
        }
    }
    profile.f0_mean = voiced_frames > 0 ? total_f0 / voiced_frames : 120.0f;

    // 3. LPC Formant Analysis (Simplified: one representative frame)
    if (num_frames > 0) {
        std::vector<float> frame(float_samples.begin() + (num_frames / 2) * frame_size_, float_samples.begin() + (num_frames / 2 + 1) * frame_size_);
        apply_hann_window(frame);
        
        int order = 14;
        std::vector<float> r(order + 1, 0.0f);
        for (int k = 0; k <= order; ++k) {
            for (int i = 0; i < (int)frame.size() - k; ++i) {
                r[k] += frame[i] * frame[i + k];
            }
        }
        
        std::vector<float> lpc = solve_levinson_durbin(r, order);
        profile.formants = lpc; // Storing LPC coeffs as a proxy for formants for now
    }

    return profile;
}

void VoiceEngine::set_preset(VoicePreset preset) {
    current_preset_ = preset;
    switch (preset) {
        case VoicePreset::Female: pitch_shift_ = 5.0f; formant_shift_ = 0.8f; break;
        case VoicePreset::Male: pitch_shift_ = -5.0f; formant_shift_ = 1.2f; break;
        case VoicePreset::Deep: pitch_shift_ = -4.0f; formant_shift_ = 1.5f; break;
        case VoicePreset::High: pitch_shift_ = 4.0f; formant_shift_ = 0.5f; break;
        case VoicePreset::Robot: pitch_shift_ = 0.0f; formant_shift_ = 1.0f; break;
        case VoicePreset::Whisper: pitch_shift_ = 0.0f; formant_shift_ = 1.0f; break;
        default: pitch_shift_ = 0.0f; formant_shift_ = 1.0f; break;
    }
}

void VoiceEngine::set_pitch_shift(float semitones) { pitch_shift_ = semitones; }
void VoiceEngine::set_formant_shift(float scale) { formant_shift_ = scale; }

void VoiceEngine::process_frame(const int16_t* input, int16_t* output, int num_samples) {
    // Handle AI Clone preset with HiFi-GAN vocoder
    if (current_preset_ == VoicePreset::AIClone && vocoder_ && is_clone_ready_) {
        // Extract voice profile features
        float f0 = current_profile_.f0_mean;
        const auto& formants = current_profile_.formants;
        const auto& cepstral = current_profile_.cepstral_coeffs;
        
        // Synthesize audio using HiFi-GAN vocoder
        std::vector<int16_t> synthesized(num_samples);
        vocoder_->synthesize_from_profile(f0, formants, cepstral, synthesized);
        
        // Copy synthesized audio to output
        std::copy(synthesized.begin(), synthesized.end(), output);
        return;
    }
    
    if (current_preset_ == VoicePreset::Natural && pitch_shift_ == 0.0f && formant_shift_ == 1.0f) {
        for (int i = 0; i < num_samples; ++i) output[i] = input[i];
        return;
    }

    // Simplified Pitch Shifting (Time-domain resampling for demonstration)
    // Real PSOLA requires pitch detection and overlap-add which is complex for a single file implementation
    // We will implement a high-quality resampling + linear interpolation for pitch
    float ratio = powf(2.0f, pitch_shift_ / 12.0f);
    
    for (int i = 0; i < num_samples; ++i) {
        float virtual_idx = i * ratio;
        int idx1 = (int)virtual_idx;
        int idx2 = std::min(idx1 + 1, num_samples - 1);
        float frac = virtual_idx - idx1;

        if (idx1 < num_samples) {
            float val = (1.0f - frac) * input[idx1] + frac * input[idx2];
            
            // Apply Robot effect (Ring Modulation)
            if (current_preset_ == VoicePreset::Robot) {
                val *= sinf(2.0f * M_PI * 50.0f * i / sample_rate_);
            }
            
            // Apply Whisper effect (Noise injection)
            if (current_preset_ == VoicePreset::Whisper) {
                float noise = ((float)rand() / RAND_MAX) * 0.2f;
                val = val * 0.5f + noise * 32767.0f * 0.5f;
            }

            output[i] = static_cast<int16_t>(std::clamp(val, -32768.0f, 32767.0f));
        } else {
            output[i] = 0;
        }
    }
}

void VoiceEngine::set_noise_suppression(bool enabled) { noise_suppression_enabled_ = enabled; }
void VoiceEngine::set_echo_cancellation(bool enabled) { echo_cancellation_enabled_ = enabled; }

} // namespace ay_pilot
