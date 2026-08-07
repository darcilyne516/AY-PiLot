# HiFi-GAN Model Setup & Conversion — Technical Deep-Dive

## Table of Contents
1. [Model Architecture Overview](#model-architecture-overview)
2. [Pre-trained Model Sources](#pre-trained-model-sources)
3. [PyTorch Model Loading](#pytorch-model-loading)
4. [ONNX Conversion](#onnx-conversion)
5. [TensorFlow Lite Conversion](#tensorflow-lite-conversion)
6. [Quantization Strategies](#quantization-strategies)
7. [Deployment Configuration](#deployment-configuration)
8. [Verification & Testing](#verification--testing)

---

## Model Architecture Overview

### HiFi-GAN v1 Architecture

HiFi-GAN is a generative adversarial network (GAN) designed specifically for neural vocoding. The architecture consists of:

**Generator (Vocoder):**
- Input: Mel-spectrogram (80 mel bins, 256 FFT, 48 kHz)
- Output: Raw waveform (16-bit PCM audio)
- Architecture: Multi-scale dilated convolutions with residual connections

**Discriminator (Training only):**
- Multi-scale discriminator (MSD) — operates on different scales
- Multi-period discriminator (MPD) — operates on different periods
- Used during training; not needed for inference

### Model Specifications

```
Input Shape:  [batch_size, 80, num_frames]
              - batch_size: 1 (real-time inference)
              - 80: mel-spectrogram bins
              - num_frames: variable (typically 256-512 frames)

Output Shape: [batch_size, num_samples]
              - num_samples: num_frames * hop_length (typically 256 * 256 = 65536)
              - At 48 kHz: ~1.37 seconds of audio

Model Size:   ~100 MB (full precision FP32)
              ~50 MB (half precision FP16)
              ~25-30 MB (quantized INT8)

Parameters:   ~13.9 million
```

### Key Components

**1. Mel-Spectrogram Encoder**
```
Input (80 channels) 
    ↓
Conv1d (80 → 512, kernel=7, padding=3)
    ↓
LeakyReLU (negative_slope=0.1)
    ↓
Output (512 channels)
```

**2. Multi-Scale Residual Blocks**
- 3 residual blocks with increasing dilation rates
- Each block: Conv1d → Dilated Conv1d → Residual Connection
- Dilation factors: [1, 3, 9], [1, 3, 9], [1, 3, 9]

**3. Transposed Convolution Upsampling**
```
Upsampling factors: [8, 8, 2, 2]
Total upsampling: 8 × 8 × 2 × 2 = 256×
Mel-spectrogram frames → Audio samples
```

**4. Output Layer**
```
Conv1d (512 → 1, kernel=7, padding=3)
    ↓
Tanh activation (output range: [-1, 1])
    ↓
Audio waveform
```

---

## Pre-trained Model Sources

### Official HiFi-GAN Repository

**Repository:** https://github.com/jik876/hifi-gan

**Available Models:**
1. **generator_v1** — Universal model (recommended)
   - Trained on: VCTK, LibriTTS, LJSpeech
   - Best for: General-purpose voice synthesis
   - Size: ~100 MB
   - Quality: MOS 4.2

2. **generator_v2** — Improved universal model
   - Trained on: Extended dataset
   - Best for: Higher quality synthesis
   - Size: ~100 MB
   - Quality: MOS 4.5

3. **generator_v3** — Fine-tuned for specific domains
   - Variants: VCTK, LibriTTS, LJSpeech
   - Best for: Domain-specific synthesis
   - Size: ~100 MB each
   - Quality: MOS 4.3-4.6

### Download Instructions

```bash
# Clone repository
git clone https://github.com/jik876/hifi-gan.git
cd hifi-gan

# Download pre-trained models
mkdir -p checkpoints

# Option 1: Download v1 (recommended for AY Pilot)
wget https://github.com/jik876/hifi-gan/releases/download/v1/generator_v1 \
  -O checkpoints/generator_v1

# Option 2: Download v2
wget https://github.com/jik876/hifi-gan/releases/download/v2/generator_v2 \
  -O checkpoints/generator_v2

# Option 3: Download from Hugging Face (alternative)
# https://huggingface.co/facebook/hifi-gan-universal
```

### Model File Structure

```
checkpoints/
├── generator_v1
│   ├── pytorch_model.bin (100 MB)
│   └── config.json
└── generator_v2
    ├── pytorch_model.bin (100 MB)
    └── config.json
```

---

## PyTorch Model Loading

### Step 1: Install Dependencies

```bash
# Create virtual environment
python3 -m venv hifigan_env
source hifigan_env/bin/activate

# Install PyTorch (CPU or GPU)
# CPU version
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu

# GPU version (CUDA 11.8)
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118

# Install additional dependencies
pip install numpy scipy librosa
```

### Step 2: Load Pre-trained Model

```python
import torch
import json
from pathlib import Path

# Define model architecture (from HiFi-GAN repository)
class Generator(torch.nn.Module):
    def __init__(self, config):
        super(Generator, self).__init__()
        self.num_kernels = len(config['resblock_kernel_sizes'])
        self.conv_pre = torch.nn.Conv1d(
            80, config['upsample_initial_out_ch'],
            kernel_size=7, padding=3
        )
        
        # Upsampling layers
        self.ups = torch.nn.ModuleList()
        for i, (u, k) in enumerate(zip(config['upsample_rates'], 
                                        config['upsample_kernel_sizes'])):
            self.ups.append(torch.nn.ConvTranspose1d(
                config['upsample_initial_out_ch'] // (2**i),
                config['upsample_initial_out_ch'] // (2**(i+1)),
                k, u, padding=(k-u)//2
            ))
        
        # Residual blocks
        self.resblocks = torch.nn.ModuleList()
        for i in range(len(self.ups)):
            ch = config['upsample_initial_out_ch'] // (2**(i+1))
            for j, (k, d) in enumerate(zip(config['resblock_kernel_sizes'],
                                           config['resblock_dilation_sizes'])):
                self.resblocks.append(ResBlock(ch, k, d))
        
        # Output layer
        self.conv_post = torch.nn.Conv1d(ch, 1, kernel_size=7, padding=3)
        self.tanh = torch.nn.Tanh()
    
    def forward(self, x):
        x = self.conv_pre(x)
        for i, ups in enumerate(self.ups):
            x = torch.nn.functional.leaky_relu(x, 0.1)
            x = ups(x)
            xs = None
            for j in range(self.num_kernels):
                if xs is None:
                    xs = self.resblocks[i*self.num_kernels + j](x)
                else:
                    xs += self.resblocks[i*self.num_kernels + j](x)
            x = xs / self.num_kernels
        x = torch.nn.functional.leaky_relu(x, 0.1)
        x = self.conv_post(x)
        x = self.tanh(x)
        return x

# Load configuration
config_path = 'checkpoints/generator_v1/config.json'
with open(config_path) as f:
    config = json.load(f)

# Create model
model = Generator(config)

# Load pre-trained weights
checkpoint = torch.load('checkpoints/generator_v1/pytorch_model.bin')
model.load_state_dict(checkpoint['generator'])

# Set to evaluation mode
model.eval()

print(f"Model loaded successfully")
print(f"Parameters: {sum(p.numel() for p in model.parameters()):,}")
print(f"Model size: {sum(p.numel() * 4 for p in model.parameters()) / 1e6:.2f} MB (FP32)")
```

### Step 3: Test Model Inference

```python
import numpy as np

# Create dummy mel-spectrogram
batch_size = 1
num_frames = 256
mel_spectrogram = torch.randn(batch_size, 80, num_frames)

# Run inference
with torch.no_grad():
    audio = model(mel_spectrogram)

print(f"Input shape: {mel_spectrogram.shape}")
print(f"Output shape: {audio.shape}")
print(f"Output range: [{audio.min():.3f}, {audio.max():.3f}]")

# Expected output: [1, 65536] (256 frames × 256 hop_length)
```

---

## ONNX Conversion

### What is ONNX?

**ONNX (Open Neural Network Exchange)** is an open standard for representing machine learning models. Benefits:

- **Cross-platform:** Works on Windows, macOS, Linux
- **Framework-agnostic:** Convert from PyTorch, TensorFlow, etc.
- **Runtime-optimized:** ONNX Runtime provides optimized inference
- **Hardware acceleration:** GPU, NPU, DSP support
- **Model optimization:** Quantization, pruning, fusion

### Step 1: Install ONNX Tools

```bash
pip install onnx onnxruntime onnx-simplifier
```

### Step 2: Convert PyTorch to ONNX

```python
import torch
import onnx

# Load model (from previous step)
model.eval()

# Create dummy input
dummy_input = torch.randn(1, 80, 256)

# Export to ONNX
torch.onnx.export(
    model,
    dummy_input,
    'hifigan_v1.onnx',
    input_names=['mel_spectrogram'],
    output_names=['audio'],
    opset_version=12,  # ONNX opset version
    do_constant_folding=True,  # Optimize constant expressions
    verbose=False,
    dynamic_axes={
        'mel_spectrogram': {0: 'batch_size', 2: 'num_frames'},
        'audio': {0: 'batch_size', 1: 'num_samples'}
    }
)

print("ONNX model exported successfully")

# Verify ONNX model
onnx_model = onnx.load('hifigan_v1.onnx')
onnx.checker.check_model(onnx_model)
print("ONNX model is valid")
```

### Step 3: Simplify ONNX Model

```python
from onnxsim import simplify

# Load ONNX model
onnx_model = onnx.load('hifigan_v1.onnx')

# Simplify (removes redundant operations)
simplified_model, check = simplify(onnx_model)

if check:
    onnx.save(simplified_model, 'hifigan_v1_simplified.onnx')
    print("ONNX model simplified and saved")
else:
    print("Simplification failed")
```

### Step 4: Verify ONNX Inference

```python
import onnxruntime as ort
import numpy as np

# Load ONNX model
sess = ort.InferenceSession('hifigan_v1.onnx', 
                            providers=['CPUExecutionProvider'])

# Create input
mel_spec = np.random.randn(1, 80, 256).astype(np.float32)

# Run inference
output = sess.run(None, {'mel_spectrogram': mel_spec})

print(f"ONNX inference successful")
print(f"Output shape: {output[0].shape}")
print(f"Output range: [{output[0].min():.3f}, {output[0].max():.3f}]")
```

### ONNX Model Information

```python
import onnx

model = onnx.load('hifigan_v1.onnx')

print("=== ONNX Model Information ===")
print(f"IR Version: {model.ir_version}")
print(f"Producer: {model.producer_name}")
print(f"Opset Version: {model.opset_import[0].version}")

print("\n=== Inputs ===")
for input_tensor in model.graph.input:
    print(f"Name: {input_tensor.name}")
    print(f"Shape: {[d.dim_value for d in input_tensor.type.tensor_type.shape.dim]}")
    print(f"Type: {input_tensor.type.tensor_type.elem_type}")

print("\n=== Outputs ===")
for output_tensor in model.graph.output:
    print(f"Name: {output_tensor.name}")
    print(f"Shape: {[d.dim_value for d in output_tensor.type.tensor_type.shape.dim]}")
    print(f"Type: {output_tensor.type.tensor_type.elem_type}")

print(f"\n=== Model Size ===")
import os
size_mb = os.path.getsize('hifigan_v1.onnx') / 1e6
print(f"File size: {size_mb:.2f} MB")
```

---

## TensorFlow Lite Conversion

### Why TensorFlow Lite?

- **Mobile-optimized:** Smaller model size, faster inference
- **On-device inference:** No cloud dependency
- **Hardware acceleration:** GPU, NPU support on Android
- **Quantization-friendly:** INT8, FP16 support

### Step 1: Install TensorFlow

```bash
pip install tensorflow tensorflow-addons tf2onnx
```

### Step 2: Convert ONNX to TensorFlow

```python
import onnx
import onnx_tf.backend as onnx_backend
import tensorflow as tf

# Load ONNX model
onnx_model = onnx.load('hifigan_v1.onnx')

# Convert to TensorFlow
print("Converting ONNX to TensorFlow...")
tf_rep = onnx_backend.prepare(onnx_model)

# Export to SavedModel format
tf_rep.export_graph('hifigan_model')
print("TensorFlow model exported to hifigan_model/")
```

### Step 3: Convert TensorFlow to TFLite

```python
import tensorflow as tf

# Load SavedModel
converter = tf.lite.TFLiteConverter.from_saved_model('hifigan_model')

# Set optimization options
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Enable experimental ops if needed
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS,
    tf.lite.OpsSet.SELECT_TF_OPS  # For operations not in TFLITE_BUILTINS
]

# Set target devices
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS
]

# Convert
print("Converting to TFLite...")
tflite_model = converter.convert()

# Save TFLite model
with open('hifigan_v1.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"TFLite model saved: hifigan_v1.tflite")
print(f"File size: {len(tflite_model) / 1e6:.2f} MB")
```

### Step 4: Verify TFLite Inference

```python
import tensorflow as tf
import numpy as np

# Load TFLite model
interpreter = tf.lite.Interpreter(model_path='hifigan_v1.tflite')
interpreter.allocate_tensors()

# Get input/output details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("=== TFLite Model Details ===")
print(f"Input: {input_details[0]['name']}, shape: {input_details[0]['shape']}")
print(f"Output: {output_details[0]['name']}, shape: {output_details[0]['shape']}")

# Create input
mel_spec = np.random.randn(1, 80, 256).astype(np.float32)

# Run inference
interpreter.set_tensor(input_details[0]['index'], mel_spec)
interpreter.invoke()

# Get output
output = interpreter.get_tensor(output_details[0]['index'])

print(f"\nTFLite inference successful")
print(f"Output shape: {output.shape}")
print(f"Output range: [{output.min():.3f}, {output.max():.3f}]")
```

---

## Quantization Strategies

### Why Quantization?

| Metric | FP32 | FP16 | INT8 |
|--------|------|------|------|
| Model Size | 100 MB | 50 MB | 25-30 MB |
| Inference Speed | 1× | 1.5-2× | 2-4× |
| Memory Usage | 100 MB | 50 MB | 25-30 MB |
| Accuracy Loss | 0% | <0.1% | 1-3% |

### Strategy 1: Post-Training Quantization (INT8)

```python
import tensorflow as tf
import numpy as np

# Prepare representative dataset
def representative_dataset():
    for _ in range(100):
        # Generate random mel-spectrograms
        mel_spec = np.random.randn(1, 80, 256).astype(np.float32)
        yield [mel_spec]

# Convert with quantization
converter = tf.lite.TFLiteConverter.from_saved_model('hifigan_model')
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset

# Full integer quantization
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8
]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

# Convert
tflite_quantized = converter.convert()

# Save
with open('hifigan_v1_quantized_int8.tflite', 'wb') as f:
    f.write(tflite_quantized)

print(f"Quantized model size: {len(tflite_quantized) / 1e6:.2f} MB")
```

### Strategy 2: Dynamic Range Quantization

```python
# Convert with dynamic range quantization
converter = tf.lite.TFLiteConverter.from_saved_model('hifigan_model')
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Dynamic range quantization (weights only)
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS
]

tflite_dynamic = converter.convert()

with open('hifigan_v1_quantized_dynamic.tflite', 'wb') as f:
    f.write(tflite_dynamic)

print(f"Dynamic range quantized model size: {len(tflite_dynamic) / 1e6:.2f} MB")
```

### Strategy 3: ONNX Quantization

```python
from onnxruntime.quantization import quantize_dynamic, QuantType

# Quantize ONNX model
quantize_dynamic(
    'hifigan_v1.onnx',
    'hifigan_v1_quantized.onnx',
    weight_type=QuantType.QInt8,
    optimize_model=True
)

print("ONNX model quantized to INT8")

# Verify quantized model
import onnxruntime as ort
sess = ort.InferenceSession('hifigan_v1_quantized.onnx')
print("Quantized ONNX model loaded successfully")
```

---

## Deployment Configuration

### Desktop Deployment (ONNX)

**Directory Structure:**
```
desktop/
├── models/
│   ├── hifigan_v1.onnx (100 MB or 50 MB quantized)
│   └── config.json
├── src/
│   └── native/
│       └── ay_pilot_napi_vocoder.cpp
└── binding.gyp
```

**Build Configuration (binding.gyp):**
```gyp
{
  "targets": [
    {
      "target_name": "ay_pilot_vocoder",
      "sources": ["src/native/ay_pilot_napi_vocoder.cpp"],
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "/path/to/onnxruntime/include"
      ],
      "libraries": [
        "-lonnxruntime"
      ],
      "library_dirs": [
        "/path/to/onnxruntime/lib"
      ]
    }
  ]
}
```

**Runtime Configuration:**
```javascript
// Electron main process
const ayPilot = require('./native/ay_pilot_vocoder.node');

// Initialize vocoder
ayPilot.initializeVocoder(
  path.join(app.getPath('appData'), 'AYPilot', 'models', 'hifigan_v1.onnx')
);
```

### Android Deployment (TFLite)

**Directory Structure:**
```
android/app/src/main/
├── assets/
│   └── models/
│       └── hifigan_v1.tflite (25-30 MB)
├── cpp/
│   ├── ay_pilot_jni_vocoder.cpp
│   └── CMakeLists.txt
└── java/com/aypilot/nativeapp/
    └── VoiceEngineVocoder.kt
```

**CMakeLists.txt Configuration:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(ay_pilot_vocoder)

# Add TensorFlow Lite
add_library(tensorflowlite STATIC IMPORTED)
set_target_properties(tensorflowlite PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/${ANDROID_ABI}/libtensorflowlite.a"
)

# Add vocoder library
add_library(ay_pilot_vocoder SHARED
  ay_pilot_jni_vocoder.cpp
)

target_link_libraries(ay_pilot_vocoder
  tensorflowlite
  log
)
```

**Gradle Configuration (build.gradle):**
```gradle
android {
    ...
    packagingOptions {
        exclude 'lib/x86/libc++_shared.so'
        exclude 'lib/x86_64/libc++_shared.so'
    }
}

dependencies {
    implementation 'org.tensorflow:tensorflow-lite:2.13.0'
    implementation 'org.tensorflow:tensorflow-lite-gpu:2.13.0'
    implementation 'org.tensorflow:tensorflow-lite-nnapi:2.13.0'
}
```

**Runtime Configuration (Kotlin):**
```kotlin
// Load model from assets
val assetManager = context.assets
val modelInputStream = assetManager.open("models/hifigan_v1.tflite")
val modelPath = File(context.cacheDir, "hifigan_v1.tflite")
modelInputStream.copyTo(modelPath.outputStream())

// Initialize vocoder
voiceEngine.initVocoder(modelPath.absolutePath)
```

---

## Verification & Testing

### Performance Benchmarking

```python
import time
import numpy as np
import onnxruntime as ort

# Load ONNX model
sess = ort.InferenceSession('hifigan_v1.onnx')

# Benchmark
num_iterations = 100
mel_spec = np.random.randn(1, 80, 256).astype(np.float32)

start_time = time.time()
for _ in range(num_iterations):
    output = sess.run(None, {'mel_spectrogram': mel_spec})
elapsed_time = time.time() - start_time

avg_latency = (elapsed_time / num_iterations) * 1000
throughput = num_iterations / elapsed_time

print(f"=== Performance Benchmark ===")
print(f"Iterations: {num_iterations}")
print(f"Total time: {elapsed_time:.2f} seconds")
print(f"Average latency: {avg_latency:.2f} ms")
print(f"Throughput: {throughput:.2f} inferences/sec")
print(f"Output samples: {output[0].shape[1]}")
print(f"Audio duration: {output[0].shape[1] / 48000:.2f} seconds")
```

### Audio Quality Verification

```python
import numpy as np
import librosa
import soundfile as sf

# Generate mel-spectrogram from reference audio
audio, sr = librosa.load('reference_audio.wav', sr=48000)
mel_spec = librosa.feature.melspectrogram(y=audio, sr=sr, n_mels=80)

# Synthesize with vocoder
output = sess.run(None, {'mel_spectrogram': mel_spec})
synthesized_audio = output[0].squeeze()

# Save synthesized audio
sf.write('synthesized_audio.wav', synthesized_audio, 48000)

# Calculate metrics
# 1. Spectral distortion
spec_original = np.abs(librosa.stft(audio))
spec_synthesized = np.abs(librosa.stft(synthesized_audio))

spectral_distortion = np.mean(np.abs(
    20 * np.log10(spec_original + 1e-8) - 
    20 * np.log10(spec_synthesized + 1e-8)
))

print(f"Spectral Distortion: {spectral_distortion:.2f} dB")

# 2. Mel-cepstral distortion
mcc_original = librosa.feature.mfcc(y=audio, sr=sr, n_mfcc=13)
mcc_synthesized = librosa.feature.mfcc(y=synthesized_audio, sr=sr, n_mfcc=13)

mcd = np.mean(np.sqrt(np.sum((mcc_original - mcc_synthesized)**2, axis=0)))
print(f"Mel-Cepstral Distortion: {mcd:.2f} dB")
```

### Cross-Platform Verification

```bash
# Test ONNX on Desktop
python3 verify_onnx.py

# Test TFLite on Android
adb push hifigan_v1.tflite /data/local/tmp/
adb shell "cd /data/local/tmp && ./test_tflite"

# Compare outputs
python3 compare_outputs.py
```

---

## Troubleshooting

### Issue: ONNX Export Fails
```
Error: Unsupported operation: aten::upsample_nearest
Solution: Use opset_version=12 or higher, or install onnx-simplifier
```

### Issue: TFLite Conversion Fails
```
Error: Cannot convert operation type 'Unsqueeze'
Solution: Use tf2onnx first, then convert ONNX to TFLite
```

### Issue: Inference Produces NaN
```
Error: Output contains NaN values
Solution: Check input range (should be normalized mel-spectrogram)
```

### Issue: Model Size Too Large
```
Solution: Apply quantization (INT8 reduces size by 75%)
```

---

## Summary

| Step | Tool | Input | Output | Time |
|------|------|-------|--------|------|
| 1. Load | PyTorch | generator_v1 | .pth | 5 min |
| 2. Export | ONNX | .pth | .onnx | 10 min |
| 3. Simplify | onnx-simplifier | .onnx | .onnx | 5 min |
| 4. Convert | tf2onnx | .onnx | SavedModel | 15 min |
| 5. Quantize | TFLite | SavedModel | .tflite | 10 min |
| 6. Verify | TFLite | .tflite | metrics | 5 min |

**Total time: ~50 minutes**

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-29  
**Difficulty Level:** Advanced
