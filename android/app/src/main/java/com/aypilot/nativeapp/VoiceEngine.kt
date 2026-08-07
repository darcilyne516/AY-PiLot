package com.aypilot.nativeapp

class VoiceEngine {
    companion object {
        init {
            System.loadLibrary("ay_pilot_native")
        }
    }

    external fun init(sampleRate: Int, frameSize: Int)
    external fun setPreset(presetIndex: Int)
    external fun process(input: ShortArray, output: ShortArray, numSamples: Int)
}
