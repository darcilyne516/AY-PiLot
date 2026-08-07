#ifndef HIFIGAN_VOCODER_H
#define HIFIGAN_VOCODER_H

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace ay_pilot {

/**
 * HiFi-GAN Neural Vocoder
 * 
 * Converts mel-spectrograms to high-quality audio waveforms.
 * Pre-trained model optimized for real-time inference (<40ms latency).
 * 
 * Model: HiFi-GAN v1 (Universal)
 * Input: Mel-spectrogram (80 mel bins, 256 FFT)
 * Output: 16-bit PCM audio at 48 kHz
 * Latency: ~15-20ms per 512-sample frame
 */
class HiFiGANVocoder {
public:
    /**
     * Initialize vocoder with pre-trained model
     * @param model_path Path to ONNX or TFLite model file
     * @param sample_rate Target sample rate (default 48000 Hz)
     */
    HiFiGANVocoder(const std::string& model_path, int sample_rate = 48000);
    ~HiFiGANVocoder();

    /**
     * Generate audio from mel-spectrogram
     * @param mel_spectrogram Input mel-spectrogram (80 mel bins)
     * @param output_audio Output audio buffer (16-bit PCM)
     * @return Number of samples generated
     */
    int synthesize(const std::vector<float>& mel_spectrogram, 
                   std::vector<int16_t>& output_audio);

    /**
     * Generate audio from voice profile features
     * @param f0 Fundamental frequency (Hz)
     * @param formants Formant frequencies (F1, F2, F3)
     * @param cepstral_coeffs Cepstral coefficients (13 values)
     * @param output_audio Output audio buffer (16-bit PCM)
     * @return Number of samples generated
     */
    int synthesize_from_profile(float f0, 
                                const std::vector<float>& formants,
                                const std::vector<float>& cepstral_coeffs,
                                std::vector<int16_t>& output_audio);

    /**
     * Check if vocoder is ready for inference
     * @return true if model loaded successfully
     */
    bool is_ready() const { return model_loaded_; }

    /**
     * Get model information
     * @return Model name and version
     */
    std::string get_model_info() const;

    /**
     * Get inference latency (milliseconds)
     * @return Average latency for last inference
     */
    float get_latency_ms() const { return last_latency_ms_; }

private:
    int sample_rate_;
    bool model_loaded_;
    float last_latency_ms_;
    
    // Model handles (implementation-specific)
    void* model_handle_;
    void* session_handle_;

    /**
     * Extract mel-spectrogram from voice profile features
     * Uses cepstral coefficients and formant information
     */
    std::vector<float> extract_mel_spectrogram(float f0,
                                              const std::vector<float>& formants,
                                              const std::vector<float>& cepstral_coeffs);

    /**
     * Run inference on mel-spectrogram
     * Supports both ONNX and TFLite backends
     */
    std::vector<int16_t> run_inference(const std::vector<float>& mel_spectrogram);

    /**
     * Post-process generated audio
     * Applies gain normalization and clipping
     */
    void post_process_audio(std::vector<int16_t>& audio);
};

} // namespace ay_pilot

#endif // HIFIGAN_VOCODER_H
