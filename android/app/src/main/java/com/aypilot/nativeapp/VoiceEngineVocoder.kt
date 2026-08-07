package com.aypilot.nativeapp

/**
 * Kotlin Wrapper for HiFi-GAN Vocoder
 * 
 * Provides high-level interface to C++ vocoder functionality
 * for Android Jetpack Compose UI.
 */

class VoiceEngineVocoder {
    
    companion object {
        init {
            System.loadLibrary("ay_pilot_engine")
        }
    }
    
    // Native methods for vocoder
    external fun initializeVocoder(modelPath: String)
    external fun isVocoderReady(): Boolean
    external fun trainClone(sample: ShortArray)
    external fun setPreset(preset: Int)
    external fun processFrameVocoder(input: ShortArray, output: ShortArray)
    external fun getVocoderLatency(): Float
    external fun getVocoderInfo(): String
    
    /**
     * Voice Preset Enum (must match C++ enum)
     */
    enum class VoicePreset(val value: Int) {
        NATURAL(0),
        FEMALE(1),
        MALE(2),
        DEEP(3),
        HIGH(4),
        ROBOT(5),
        WHISPER(6),
        AI_CLONE(7)
    }
    
    /**
     * Initialize vocoder with model file
     * @param modelPath Path to ONNX or TFLite model
     */
    fun initVocoder(modelPath: String) {
        try {
            initializeVocoder(modelPath)
        } catch (e: Exception) {
            android.util.Log.e("VoiceEngineVocoder", "Failed to initialize vocoder: ${e.message}")
        }
    }
    
    /**
     * Check if vocoder is ready for synthesis
     * @return true if vocoder model loaded and ready
     */
    fun isReady(): Boolean {
        return try {
            isVocoderReady()
        } catch (e: Exception) {
            false
        }
    }
    
    /**
     * Train AI Clone from voice sample
     * @param sample Voice sample (16-bit PCM audio)
     */
    fun trainAIClone(sample: ShortArray) {
        try {
            trainClone(sample)
        } catch (e: Exception) {
            android.util.Log.e("VoiceEngineVocoder", "Failed to train clone: ${e.message}")
        }
    }
    
    /**
     * Set voice preset
     * @param preset Voice preset (Natural, Female, Male, etc.)
     */
    fun setVoicePreset(preset: VoicePreset) {
        try {
            setPreset(preset.value)
        } catch (e: Exception) {
            android.util.Log.e("VoiceEngineVocoder", "Failed to set preset: ${e.message}")
        }
    }
    
    /**
     * Process audio frame with vocoder
     * @param input Input audio frame
     * @param output Output audio frame (transformed)
     */
    fun processFrame(input: ShortArray, output: ShortArray) {
        try {
            processFrameVocoder(input, output)
        } catch (e: Exception) {
            android.util.Log.e("VoiceEngineVocoder", "Failed to process frame: ${e.message}")
        }
    }
    
    /**
     * Get vocoder inference latency
     * @return Latency in milliseconds
     */
    fun getLatency(): Float {
        return try {
            getVocoderLatency()
        } catch (e: Exception) {
            0.0f
        }
    }
    
    /**
     * Get vocoder model information
     * @return Model name and version
     */
    fun getModelInfo(): String {
        return try {
            getVocoderInfo()
        } catch (e: Exception) {
            "Vocoder not available"
        }
    }
}

/**
 * Usage Example in Jetpack Compose:
 * 
 * @Composable
 * fun AIClonePresetButton(voiceEngine: VoiceEngineVocoder) {
 *     Button(
 *         onClick = {
 *             voiceEngine.setVoicePreset(VoiceEngineVocoder.VoicePreset.AI_CLONE)
 *             
 *             if (voiceEngine.isReady()) {
 *                 // Vocoder ready for synthesis
 *                 val latency = voiceEngine.getLatency()
 *                 Log.d("AIClone", "Vocoder latency: $latency ms")
 *             } else {
 *                 // Initialize vocoder
 *                 voiceEngine.initVocoder("models/hifigan_v1.onnx")
 *             }
 *         }
 *     ) {
 *         Text("AI Clone")
 *     }
 * }
 * 
 * @Composable
 * fun TrainAICloneDialog(voiceEngine: VoiceEngineVocoder) {
 *     var recordingState by remember { mutableStateOf(RecordingState.IDLE) }
 *     
 *     Button(
 *         onClick = {
 *             recordingState = RecordingState.RECORDING
 *             // Record 15-30 second voice sample
 *             // Then:
 *             voiceEngine.trainAIClone(voiceSample)
 *             recordingState = RecordingState.COMPLETE
 *         }
 *     ) {
 *         Text("Record Voice for AI Clone")
 *     }
 * }
 */
