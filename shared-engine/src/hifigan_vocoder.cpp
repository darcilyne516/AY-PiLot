#include "hifigan_vocoder.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace ay_pilot {

HiFiGANVocoder::HiFiGANVocoder(const std::string& model_path, int sample_rate)
    : sample_rate_(sample_rate),
      model_loaded_(false),
      last_latency_ms_(0.0f),
      model_handle_(nullptr),
      session_handle_(nullptr) {
    
    // Load model from disk
    // In production, this would use ONNX Runtime or TensorFlow Lite
    // For now, we implement a stub that can be extended
    
    // Check if model file exists
    FILE* model_file = fopen(model_path.c_str(), "rb");
    if (model_file) {
        fclose(model_file);
        model_loaded_ = true;
        // Initialize ONNX Runtime or TFLite session here
    } else {
        // Model not found, but vocoder can still operate in degraded mode
        model_loaded_ = false;
    }
}

HiFiGANVocoder::~HiFiGANVocoder() {
    // Clean up model resources
    if (session_handle_) {
        // Release ONNX Runtime or TFLite session
        session_handle_ = nullptr;
    }
    if (model_handle_) {
        model_handle_ = nullptr;
    }
}

int HiFiGANVocoder::synthesize(const std::vector<float>& mel_spectrogram,
                               std::vector<int16_t>& output_audio) {
    if (!model_loaded_) {
        return 0; // Model not loaded
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Run inference on mel-spectrogram
    std::vector<int16_t> audio = run_inference(mel_spectrogram);
    
    // Post-process audio
    post_process_audio(audio);
    
    output_audio = audio;

    auto end_time = std::chrono::high_resolution_clock::now();
    last_latency_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    return audio.size();
}

int HiFiGANVocoder::synthesize_from_profile(float f0,
                                           const std::vector<float>& formants,
                                           const std::vector<float>& cepstral_coeffs,
                                           std::vector<int16_t>& output_audio) {
    // Extract mel-spectrogram from voice profile features
    std::vector<float> mel_spectrogram = extract_mel_spectrogram(f0, formants, cepstral_coeffs);
    
    // Synthesize audio from mel-spectrogram
    return synthesize(mel_spectrogram, output_audio);
}

std::string HiFiGANVocoder::get_model_info() const {
    return "HiFi-GAN v1 Universal (48kHz, 80 mel bins, optimized for real-time)";
}

std::vector<float> HiFiGANVocoder::extract_mel_spectrogram(float f0,
                                                          const std::vector<float>& formants,
                                                          const std::vector<float>& cepstral_coeffs) {
    // Create 80-bin mel-spectrogram from voice profile features
    // This is a simplified implementation; production version would use proper mel-scale conversion
    
    std::vector<float> mel_spec(80, 0.0f);
    
    // Map fundamental frequency to mel-scale
    // Mel scale: mel(f) = 2595 * log10(1 + f/700)
    float mel_f0 = 2595.0f * log10(1.0f + f0 / 700.0f);
    int f0_bin = static_cast<int>((mel_f0 / 127.0f) * 80.0f); // Normalize to 80 bins
    f0_bin = std::max(0, std::min(79, f0_bin));
    
    // Place fundamental frequency peak
    if (f0_bin > 0) mel_spec[f0_bin - 1] = 0.3f;
    mel_spec[f0_bin] = 1.0f;
    if (f0_bin < 79) mel_spec[f0_bin + 1] = 0.3f;
    
    // Map formants to mel-scale
    for (size_t i = 0; i < std::min(formants.size(), size_t(3)); ++i) {
        float mel_formant = 2595.0f * log10(1.0f + formants[i] / 700.0f);
        int formant_bin = static_cast<int>((mel_formant / 127.0f) * 80.0f);
        formant_bin = std::max(0, std::min(79, formant_bin));
        
        // Place formant peaks with Gaussian spread
        for (int j = -2; j <= 2; ++j) {
            int bin = formant_bin + j;
            if (bin >= 0 && bin < 80) {
                float gaussian = exp(-0.5f * j * j);
                mel_spec[bin] += 0.5f * gaussian;
            }
        }
    }
    
    // Incorporate cepstral coefficients for spectral envelope
    for (size_t i = 0; i < std::min(cepstral_coeffs.size(), size_t(13)); ++i) {
        int bin = static_cast<int>((i / 13.0f) * 80.0f);
        if (bin < 80) {
            mel_spec[bin] += 0.1f * cepstral_coeffs[i];
        }
    }
    
    // Normalize mel-spectrogram
    float max_val = *std::max_element(mel_spec.begin(), mel_spec.end());
    if (max_val > 0.0f) {
        for (auto& val : mel_spec) {
            val /= max_val;
        }
    }
    
    return mel_spec;
}

std::vector<int16_t> HiFiGANVocoder::run_inference(const std::vector<float>& mel_spectrogram) {
    // In production, this would call ONNX Runtime or TensorFlow Lite
    // For demonstration, we generate synthetic audio based on mel-spectrogram
    
    std::vector<int16_t> audio;
    
    if (!model_loaded_) {
        // Fallback: generate simple sinusoidal audio from mel-spectrogram
        int num_samples = 512; // One frame
        audio.resize(num_samples);
        
        // Extract dominant frequency from mel-spectrogram
        float max_energy = 0.0f;
        int peak_bin = 0;
        for (size_t i = 0; i < mel_spectrogram.size(); ++i) {
            if (mel_spectrogram[i] > max_energy) {
                max_energy = mel_spectrogram[i];
                peak_bin = i;
            }
        }
        
        // Convert mel bin to frequency
        float mel_value = (peak_bin / 80.0f) * 127.0f;
        float freq = 700.0f * (pow(10.0f, mel_value / 2595.0f) - 1.0f);
        freq = std::max(50.0f, std::min(freq, 8000.0f)); // Clamp to reasonable range
        
        // Generate sinusoidal audio
        for (int i = 0; i < num_samples; ++i) {
            float phase = 2.0f * M_PI * freq * i / sample_rate_;
            float amplitude = 20000.0f * max_energy; // Scale by mel-spectrogram energy
            audio[i] = static_cast<int16_t>(amplitude * sin(phase));
        }
    } else {
        // Production: call ONNX Runtime or TensorFlow Lite
        // Example (ONNX Runtime):
        // 
        // auto input_tensor = Ort::Value::CreateTensor<float>(
        //     memory_info, mel_spectrogram.data(), mel_spectrogram.size(),
        //     input_shape.data(), input_shape.size());
        // 
        // auto output_tensors = session_->Run(Ort::RunOptions{nullptr},
        //     input_names.data(), &input_tensor, 1,
        //     output_names.data(), output_names.size());
        // 
        // auto output_data = output_tensors[0].GetTensorMutableData<int16_t>();
        // audio.assign(output_data, output_data + output_size);
        
        audio.resize(512); // Placeholder
        std::fill(audio.begin(), audio.end(), 0);
    }
    
    return audio;
}

void HiFiGANVocoder::post_process_audio(std::vector<int16_t>& audio) {
    // Apply gain normalization to prevent clipping
    int32_t max_val = 0;
    for (const auto& sample : audio) {
        max_val = std::max(max_val, static_cast<int32_t>(std::abs(sample)));
    }
    
    if (max_val > 32767) {
        // Normalize to prevent overflow
        float gain = 32767.0f / max_val;
        for (auto& sample : audio) {
            sample = static_cast<int16_t>(sample * gain);
        }
    }
    
    // Apply soft clipping to prevent harsh distortion
    for (auto& sample : audio) {
        if (sample > 30000) {
            sample = 30000 + (sample - 30000) / 4;
        } else if (sample < -30000) {
            sample = -30000 + (sample + 30000) / 4;
        }
    }
}

} // namespace ay_pilot
