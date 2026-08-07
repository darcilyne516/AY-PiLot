#!/usr/bin/env python3
"""
HiFi-GAN Model Conversion Script
Converts PyTorch → ONNX → TensorFlow Lite for AY Pilot Native

Usage:
    python3 convert_hifigan_model.py --model-path ./checkpoints/generator_v1 --output-dir ./models
"""

import argparse
import json
import os
import sys
from pathlib import Path

import numpy as np
import torch
import onnx
import onnxruntime as ort
import tensorflow as tf

# Suppress TensorFlow warnings
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'


class HiFiGANConverter:
    """Convert HiFi-GAN models between formats"""
    
    def __init__(self, model_path, output_dir):
        self.model_path = Path(model_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
    def load_pytorch_model(self):
        """Load pre-trained PyTorch model"""
        print("[1/6] Loading PyTorch model...")
        
        # Load configuration
        config_path = self.model_path / 'config.json'
        if not config_path.exists():
            print(f"Error: Config not found at {config_path}")
            sys.exit(1)
        
        with open(config_path) as f:
            self.config = json.load(f)
        
        # Import HiFi-GAN model class
        # Note: This requires the HiFi-GAN repository to be in PYTHONPATH
        try:
            from models import Generator
        except ImportError:
            print("Error: HiFi-GAN models not found. Clone repository:")
            print("  git clone https://github.com/jik876/hifi-gan.git")
            sys.exit(1)
        
        # Create and load model
        self.model = Generator(self.config)
        
        checkpoint_path = self.model_path / 'pytorch_model.bin'
        if not checkpoint_path.exists():
            print(f"Error: Model checkpoint not found at {checkpoint_path}")
            sys.exit(1)
        
        checkpoint = torch.load(checkpoint_path, map_location='cpu')
        self.model.load_state_dict(checkpoint['generator'])
        self.model.eval()
        
        # Count parameters
        num_params = sum(p.numel() for p in self.model.parameters())
        model_size_mb = num_params * 4 / 1e6  # FP32
        
        print(f"✓ Model loaded successfully")
        print(f"  Parameters: {num_params:,}")
        print(f"  Model size (FP32): {model_size_mb:.2f} MB")
        
        return self.model
    
    def test_pytorch_inference(self):
        """Test PyTorch model inference"""
        print("\n[2/6] Testing PyTorch inference...")
        
        # Create dummy input
        dummy_input = torch.randn(1, 80, 256)
        
        # Run inference
        with torch.no_grad():
            output = self.model(dummy_input)
        
        print(f"✓ PyTorch inference successful")
        print(f"  Input shape: {dummy_input.shape}")
        print(f"  Output shape: {output.shape}")
        print(f"  Output range: [{output.min():.3f}, {output.max():.3f}]")
        
        return dummy_input, output
    
    def export_to_onnx(self, dummy_input):
        """Export PyTorch model to ONNX"""
        print("\n[3/6] Exporting to ONNX...")
        
        onnx_path = self.output_dir / 'hifigan_v1.onnx'
        
        # Export
        torch.onnx.export(
            self.model,
            dummy_input,
            str(onnx_path),
            input_names=['mel_spectrogram'],
            output_names=['audio'],
            opset_version=12,
            do_constant_folding=True,
            verbose=False,
            dynamic_axes={
                'mel_spectrogram': {0: 'batch_size', 2: 'num_frames'},
                'audio': {0: 'batch_size', 1: 'num_samples'}
            }
        )
        
        # Verify ONNX model
        onnx_model = onnx.load(str(onnx_path))
        onnx.checker.check_model(onnx_model)
        
        # Get file size
        file_size_mb = os.path.getsize(onnx_path) / 1e6
        
        print(f"✓ ONNX export successful")
        print(f"  Path: {onnx_path}")
        print(f"  File size: {file_size_mb:.2f} MB")
        
        return str(onnx_path)
    
    def test_onnx_inference(self, onnx_path):
        """Test ONNX model inference"""
        print("\n[4/6] Testing ONNX inference...")
        
        # Load ONNX model
        sess = ort.InferenceSession(onnx_path, providers=['CPUExecutionProvider'])
        
        # Create dummy input
        mel_spec = np.random.randn(1, 80, 256).astype(np.float32)
        
        # Run inference
        output = sess.run(None, {'mel_spectrogram': mel_spec})
        
        print(f"✓ ONNX inference successful")
        print(f"  Input shape: {mel_spec.shape}")
        print(f"  Output shape: {output[0].shape}")
        print(f"  Output range: [{output[0].min():.3f}, {output[0].max():.3f}]")
        
        return onnx_path
    
    def convert_to_tensorflow(self, onnx_path):
        """Convert ONNX to TensorFlow SavedModel"""
        print("\n[5/6] Converting to TensorFlow...")
        
        try:
            import onnx_tf.backend as onnx_backend
        except ImportError:
            print("Error: onnx-tf not installed. Install with:")
            print("  pip install onnx-tf")
            sys.exit(1)
        
        # Load ONNX model
        onnx_model = onnx.load(onnx_path)
        
        # Convert to TensorFlow
        tf_rep = onnx_backend.prepare(onnx_model)
        
        # Export to SavedModel
        tf_model_path = self.output_dir / 'hifigan_model'
        tf_rep.export_graph(str(tf_model_path))
        
        print(f"✓ TensorFlow conversion successful")
        print(f"  Path: {tf_model_path}")
        
        return str(tf_model_path)
    
    def convert_to_tflite(self, tf_model_path):
        """Convert TensorFlow SavedModel to TFLite"""
        print("\n[6/6] Converting to TensorFlow Lite...")
        
        # Load SavedModel
        converter = tf.lite.TFLiteConverter.from_saved_model(tf_model_path)
        
        # Set optimization options
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        
        # Support both TFLITE_BUILTINS and SELECT_TF_OPS
        converter.target_spec.supported_ops = [
            tf.lite.OpsSet.TFLITE_BUILTINS,
            tf.lite.OpsSet.SELECT_TF_OPS
        ]
        
        # Convert
        tflite_model = converter.convert()
        
        # Save TFLite model
        tflite_path = self.output_dir / 'hifigan_v1.tflite'
        with open(tflite_path, 'wb') as f:
            f.write(tflite_model)
        
        # Get file size
        file_size_mb = len(tflite_model) / 1e6
        
        print(f"✓ TFLite conversion successful")
        print(f"  Path: {tflite_path}")
        print(f"  File size: {file_size_mb:.2f} MB")
        
        return str(tflite_path)
    
    def quantize_onnx(self, onnx_path):
        """Quantize ONNX model to INT8"""
        print("\nQuantizing ONNX model to INT8...")
        
        try:
            from onnxruntime.quantization import quantize_dynamic, QuantType
        except ImportError:
            print("Warning: ONNX quantization tools not available")
            return None
        
        quantized_path = self.output_dir / 'hifigan_v1_quantized.onnx'
        
        quantize_dynamic(
            onnx_path,
            str(quantized_path),
            weight_type=QuantType.QInt8,
            optimize_model=True
        )
        
        # Get file sizes
        original_size = os.path.getsize(onnx_path) / 1e6
        quantized_size = os.path.getsize(quantized_path) / 1e6
        compression_ratio = (1 - quantized_size / original_size) * 100
        
        print(f"✓ ONNX quantization successful")
        print(f"  Original size: {original_size:.2f} MB")
        print(f"  Quantized size: {quantized_size:.2f} MB")
        print(f"  Compression: {compression_ratio:.1f}%")
        
        return str(quantized_path)
    
    def convert(self):
        """Run complete conversion pipeline"""
        print("=" * 60)
        print("HiFi-GAN Model Conversion Pipeline")
        print("=" * 60)
        
        try:
            # Step 1: Load PyTorch model
            self.load_pytorch_model()
            
            # Step 2: Test PyTorch inference
            dummy_input, _ = self.test_pytorch_inference()
            
            # Step 3: Export to ONNX
            onnx_path = self.export_to_onnx(dummy_input)
            
            # Step 4: Test ONNX inference
            self.test_onnx_inference(onnx_path)
            
            # Step 5: Convert to TensorFlow
            tf_model_path = self.convert_to_tensorflow(onnx_path)
            
            # Step 6: Convert to TFLite
            tflite_path = self.convert_to_tflite(tf_model_path)
            
            # Optional: Quantize ONNX
            self.quantize_onnx(onnx_path)
            
            # Summary
            print("\n" + "=" * 60)
            print("Conversion Complete!")
            print("=" * 60)
            print(f"\nOutput files:")
            print(f"  ONNX (Desktop):  {self.output_dir / 'hifigan_v1.onnx'}")
            print(f"  TFLite (Android): {self.output_dir / 'hifigan_v1.tflite'}")
            print(f"\nNext steps:")
            print(f"  1. Copy ONNX to: desktop/models/")
            print(f"  2. Copy TFLite to: android/app/src/main/assets/models/")
            print(f"  3. Build and test on target platforms")
            
        except Exception as e:
            print(f"\n✗ Error: {e}")
            import traceback
            traceback.print_exc()
            sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='Convert HiFi-GAN PyTorch model to ONNX and TFLite'
    )
    parser.add_argument(
        '--model-path',
        required=True,
        help='Path to HiFi-GAN checkpoint directory (containing pytorch_model.bin and config.json)'
    )
    parser.add_argument(
        '--output-dir',
        default='./models',
        help='Output directory for converted models (default: ./models)'
    )
    
    args = parser.parse_args()
    
    converter = HiFiGANConverter(args.model_path, args.output_dir)
    converter.convert()


if __name__ == '__main__':
    main()
