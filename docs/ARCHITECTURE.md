# AY Pilot Native — Architecture Documentation

## System Overview

AY Pilot Native is a modular, cross-platform voice transformation system composed of six interconnected layers. This document describes how each layer functions and how they integrate.

## Layer 1: Audio Engine

### Responsibilities
- Capture microphone input at 48 kHz, 16-bit PCM
- Buffer audio into 512-sample frames (10.67 ms at 48 kHz)
- Route processed audio to virtual microphone output
- Maintain sub-40ms end-to-end latency

### Platform-Specific Implementations

**Android (Oboe Library):**
- Uses Oboe for low-latency audio I/O
- Requests AAudio if available (Android 8+), falls back to OpenSL ES
- Manages audio focus and session routing
- Integrates with Android's audio policy manager for virtual microphone selection

**Desktop (Native APIs):**
- **Windows:** WASAPI (Windows Audio Session API) for capture/playback
- **macOS:** Core Audio framework
- **Linux:** ALSA or PulseAudio depending on system configuration

### Latency Breakdown
- Microphone capture: 5–10 ms
- Frame buffering: 10.67 ms (512 samples @ 48 kHz)
- Processing (Voice Transformation): 15–20 ms
- Virtual microphone output: 5–10 ms
- **Total: 35–50 ms** (target <40 ms)

## Layer 2: Voice Analysis Engine

### Responsibilities
- Analyze a 15–30 second voice sample
- Extract fundamental frequency (F0)
- Extract formant frequencies (F1, F2, F3)
- Compute cepstral coefficients for spectral envelope
- Generate an encrypted voice profile for storage

### Algorithm Details

**Pre-Emphasis Filter:**
```
y[n] = x[n] - 0.97 * x[n-1]
```
Reduces low-frequency bias and emphasizes higher frequencies for better formant extraction.

**Windowing:**
Hann window applied to each frame:
```
w[n] = 0.5 * (1 - cos(2π * n / (N-1)))
```

**F0 Extraction (Autocorrelation Method):**
1. Compute autocorrelation for lags in 50–400 Hz range
2. Find peak autocorrelation value
3. Convert lag to frequency: F0 = sample_rate / lag
4. Average F0 across voiced frames

**LPC Formant Analysis:**
1. Compute autocorrelation coefficients (order 14)
2. Solve Levinson-Durbin recursion to get LPC coefficients
3. Extract formant frequencies from LPC polynomial roots
4. Typically yields F1 (300–800 Hz), F2 (700–2500 Hz), F3 (2500–3500 Hz)

**Cepstral Analysis:**
1. Compute 13 cepstral coefficients from log spectrum
2. Captures spectral envelope independent of pitch
3. Used for voice profile matching and AI Clone synthesis

### Output: VoiceProfile Structure
```cpp
struct VoiceProfile {
    std::string name;                      // Profile name
    float f0_mean;                         // Average F0 in Hz
    std::vector<float> formants;           // F1, F2, F3 + LPC coeffs
    std::vector<float> cepstral_coeffs;    // 13 cepstral coefficients
    std::vector<uint8_t> encrypted_data;   // AES-256 encrypted profile
};
```

## Layer 3: Voice Transformation Engine

### Responsibilities
- Apply real-time pitch shifting using PSOLA or time-domain resampling
- Apply formant shifting via spectral envelope warping
- Implement 8 voice presets
- Maintain sub-40ms processing latency per frame
- Support HiFi-GAN neural vocoder for AI Clone mode

### HiFi-GAN Neural Vocoder Integration
The AI Clone preset now uses a production-grade HiFi-GAN neural vocoder:
- **Model:** HiFi-GAN v1 Universal (pre-trained on diverse voices)
- **Input:** Mel-spectrogram (80 bins, 256 FFT, 48 kHz)
- **Output:** 16-bit PCM audio at 48 kHz
- **Latency:** 15-20 ms per frame (well under 40 ms budget)
- **Quality:** MOS > 4.0, natural-sounding speech synthesis
- **Formats:** ONNX (Desktop), TensorFlow Lite (Android)

### Transformation Algorithms

**Pitch Shifting (Simplified PSOLA):**
1. Detect pitch periods from input signal
2. Resample audio based on pitch shift factor: `ratio = 2^(semitones/12)`
3. Apply linear interpolation for smooth resampling
4. Overlap-add frames to maintain continuity

**Formant Shifting:**
1. Extract spectral envelope via LPC or cepstral analysis
2. Warp frequency axis: `f_warped = f_original * formant_shift`
3. Reconstruct signal with warped envelope
4. Combine with shifted pitch for final output

**Preset Implementation:**
Each preset combines pitch and formant shifts:
- **Female:** +5 semitones, 0.8x formant (higher pitch, higher formants)
- **Male:** -5 semitones, 1.2x formant (lower pitch, lower formants)
- **Robot:** Ring modulation at 50 Hz carrier frequency
- **Whisper:** Amplitude modulation + noise injection

### Processing Pipeline
```
Input Frame (512 samples)
    ↓
[Pitch Shift] → [Formant Shift] → [Preset Effects]
    ↓
Output Frame (512 samples)
```

## Layer 4: Virtual Microphone Routing

### Responsibilities
- Capture transformed audio from Layer 3
- Route to platform-specific virtual microphone device
- Ensure seamless integration with calling applications
- Handle device selection and fallback logic

### Platform-Specific Routing

**Windows (VB-Cable):**
- Output to WASAPI endpoint: "CABLE Input"
- User selects "CABLE Output" in Signal/Discord/Zoom
- Audio flows: AY Pilot → VB-Cable → Calling App

**macOS (BlackHole):**
- Output to Core Audio device: "BlackHole 2ch"
- User selects "BlackHole 2ch" in Signal/Discord/Zoom
- Audio flows: AY Pilot → BlackHole → Calling App

**Linux (PulseAudio):**
- Output to null sink: "AYPilot"
- User selects "AYPilot" in Signal/Discord/Zoom
- Audio flows: AY Pilot → PulseAudio → Calling App

**Android (Standard Audio Session):**
- Uses AudioTrack for playback to virtual microphone
- Integrates with Android's audio routing policy
- User selects "AY Pilot" in Signal app's audio settings

## Layer 5: UI Layer

### Android (Jetpack Compose)
- **Components:** Voice preset buttons, latency meter, CPU monitor, device selector
- **State Management:** ViewModel + LiveData for reactive updates
- **Design System:** Signal Glass — dark navy background, electric violet accents, phosphor green status indicators
- **Navigation:** Single-screen app with modal dialogs for settings

### Desktop (Electron + React)
- **Components:** Voice preset grid, real-time visualizers (oscilloscope, spectrogram), device selectors
- **State Management:** React hooks + context API
- **Design System:** Same Signal Glass CSS as web version
- **Navigation:** Tabbed interface (Main, Settings, Help)

### Shared Design Principles
- **Frosted Glass Panels:** Semi-transparent backgrounds with backdrop blur
- **Electric Violet Accents:** oklch(0.62 0.28 295) for all interactive elements
- **Phosphor Green Indicators:** #00FF41 for "live," "active," "recording" states only
- **Typography:** Space Grotesk (headings), JetBrains Mono (technical readouts), Inter (body)

## Layer 6: Storage Layer

### Responsibilities
- Persist voice profiles with AES-256 encryption
- Manage profile lifecycle (create, load, rename, delete)
- Enforce maximum 20 profiles per user
- Ensure zero plaintext audio data on disk

### Android Storage
- **EncryptedSharedPreferences:** For simple key-value profile metadata
- **Room Database + SQLCipher:** For complex profile data and history
- **Location:** `/data/data/com.aypilot.nativeapp/files/` (app-private, encrypted)

### Desktop Storage
- **Encrypted JSON Files:** Stored in user's app data directory
- **Windows:** `%APPDATA%/AYPilot/profiles/`
- **macOS:** `~/Library/Application Support/AYPilot/profiles/`
- **Linux:** `~/.config/aypilot/profiles/`
- **Encryption:** AES-256 with user-derived key (PBKDF2)

### Profile Schema
```json
{
  "id": "profile_uuid",
  "name": "My Voice",
  "f0_mean": 120.5,
  "formants": [300, 700, 2500],
  "cepstral_coeffs": [...],
  "created_at": "2026-07-29T12:00:00Z",
  "encrypted_data": "base64_encoded_aes256_ciphertext"
}
```

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│ User speaks into microphone (Signal, Discord, Zoom, etc.)   │
└────────────────────┬────────────────────────────────────────┘
                     ↓
        ┌────────────────────────┐
        │   Layer 1: Audio       │
        │   Engine              │
        │ (Capture 48kHz PCM)    │
        └────────┬───────────────┘
                 ↓
        ┌────────────────────────┐
        │  Layer 3: Voice        │
        │  Transformation        │
        │ (PSOLA + Formant)      │
        └────────┬───────────────┘
                 ↓
        ┌────────────────────────┐
        │  Layer 4: Virtual      │
        │  Microphone Routing    │
        │ (VB-Cable/BlackHole)   │
        └────────┬───────────────┘
                 ↓
┌─────────────────────────────────────────────────────────────┐
│ Calling app receives transformed voice as microphone input  │
└─────────────────────────────────────────────────────────────┘
```

## Integration Points

### Android ↔ Desktop
- **Shared Engine:** Both platforms use identical C++ voice processing engine
- **JNI (Android) / N-API (Desktop):** Language bindings to C++ engine
- **Profile Format:** Both platforms use same encrypted JSON profile schema

### UI ↔ Engine
- **Android:** Kotlin UI → JNI → C++ Engine
- **Desktop:** React UI → Node.js N-API → C++ Engine
- **Latency:** UI updates happen asynchronously; audio processing is real-time

### Storage ↔ Engine
- **Profile Loading:** Engine loads encrypted profile, decrypts, and initializes transformation parameters
- **Profile Saving:** Engine extracts voice characteristics, encrypts, and persists to disk

## Performance Considerations

### CPU Optimization
- Frame-based processing (512 samples) reduces context switching
- SIMD operations for pitch shifting and formant warping (future optimization)
- Lazy initialization of neural vocoder (only when AI Clone preset is selected)

### Memory Management
- Fixed-size buffers (no dynamic allocation during audio processing)
- Profile cache (max 20 profiles) fits in <10 MB
- Audio buffers: 512 samples × 2 bytes × 2 (input + output) = 2 KB per frame

### Latency Budget
- Target: <40 ms end-to-end
- Allocation: 10 ms capture + 10 ms processing + 10 ms output + 10 ms margin
- Monitoring: Real-time latency meter in UI

## Security Model

### Threat Model
1. **Eavesdropping:** Attacker intercepts audio in transit
   - **Mitigation:** All processing local; no network transmission
2. **Profile Theft:** Attacker accesses stored voice profiles
   - **Mitigation:** AES-256 encryption with user-derived key
3. **Malicious App Integration:** Malicious calling app captures transformed audio
   - **Mitigation:** Uses standard OS audio APIs; no special privileges required
4. **Supply Chain:** Compromised dependency introduces backdoor
   - **Mitigation:** Minimal dependencies; open-source code for audit

### Encryption Details
- **Algorithm:** AES-256-CBC
- **Key Derivation:** PBKDF2 with 100,000 iterations
- **IV:** Random 16-byte IV per profile
- **Authentication:** HMAC-SHA256 for integrity verification

## Testing Strategy

### Unit Tests
- Voice analysis engine (F0 extraction, LPC, cepstral analysis)
- Pitch shifting algorithms
- Encryption/decryption functions

### Integration Tests
- Full audio pipeline latency measurement
- Virtual microphone routing with Signal/Discord/Zoom
- Profile creation and loading
- 60-minute call stability

### Performance Tests
- CPU usage during active transformation
- Memory footprint over time
- Latency consistency across different devices

## Future Enhancements

1. **Real-Time Pitch Detection Visualization:** Oscilloscope and spectrogram in UI
2. **Custom Preset Creation:** Allow users to define custom pitch/formant combinations
3. **Cloud Profile Sync:** Optional, encrypted profile synchronization across devices
4. **Streaming Platform Integration:** Direct integration with Twitch, YouTube for streamers
5. **Advanced Neural Vocoder:** Production-grade TensorFlow Lite model for AI Clone
6. **Multi-Language Support:** UI localization for major languages
7. **Accessibility Features:** Screen reader support, keyboard navigation

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-29  
**Author:** AY Pilot Development Team
