# Android Local Build Testing Guide

## Overview

This guide walks you through setting up your local development environment and building the AY Pilot Native Android app using Gradle.

---

## Prerequisites

### System Requirements

- **OS:** macOS, Linux, or Windows
- **RAM:** Minimum 8 GB (16 GB recommended)
- **Disk Space:** 20 GB free
- **Internet:** High-speed connection (for downloading dependencies)

### Software Requirements

1. **Java Development Kit (JDK) 11+**
   - Required for Gradle and Android build tools
   - Download: https://adoptopenjdk.net/ or https://www.oracle.com/java/technologies/downloads/

2. **Android SDK**
   - Required for Android build tools and emulator
   - Download: https://developer.android.com/studio

3. **Android NDK (r25b or later)**
   - Required for JNI compilation
   - Download: https://developer.android.com/ndk/downloads

4. **Git**
   - For cloning the repository
   - Download: https://git-scm.com/

---

## Installation

### Step 1: Install JDK 11

**macOS:**
```bash
brew install openjdk@11
export JAVA_HOME=$(/usr/libexec/java_home -v 11)
echo 'export JAVA_HOME=$(/usr/libexec/java_home -v 11)' >> ~/.zshrc
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install -y openjdk-11-jdk
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
echo 'export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64' >> ~/.bashrc
```

**Windows:**
- Download from: https://adoptopenjdk.net/
- Run installer
- Add JAVA_HOME to environment variables

### Step 2: Install Android SDK

**Option A: Android Studio (Recommended)**
1. Download: https://developer.android.com/studio
2. Run installer
3. Follow setup wizard
4. SDK Manager will install Android SDK automatically

**Option B: Command Line Tools**
```bash
# Download and extract
mkdir -p ~/Android/sdk
cd ~/Android/sdk
wget https://dl.google.com/android/repository/commandlinetools-linux-8512546_latest.zip
unzip commandlinetools-linux-8512546_latest.zip

# Set environment variable
export ANDROID_SDK_ROOT=~/Android/sdk
echo 'export ANDROID_SDK_ROOT=~/Android/sdk' >> ~/.bashrc
```

### Step 3: Install Android NDK

```bash
# Using Android Studio SDK Manager (easiest)
# Open Android Studio → SDK Manager → SDK Tools → NDK (Side by side)

# Or manually download
mkdir -p ~/Android/ndk
cd ~/Android/ndk
wget https://dl.google.com/android/repository/android-ndk-r25b-linux.zip
unzip android-ndk-r25b-linux.zip

# Set environment variable
export ANDROID_NDK_HOME=~/Android/ndk/android-ndk-r25b
echo 'export ANDROID_NDK_HOME=~/Android/ndk/android-ndk-r25b' >> ~/.bashrc
```

### Step 4: Verify Installation

```bash
# Check Java
java -version
# Output: openjdk version "11.0.x"

# Check Android SDK
ls $ANDROID_SDK_ROOT/platforms/
# Output: android-29, android-30, android-31, ...

# Check Android NDK
ls $ANDROID_NDK_HOME/
# Output: ndk-build, toolchains, ...
```

---

## Project Setup

### Clone Repository

```bash
git clone https://github.com/darcilyne516/AY-PiLot.git
cd AY-PiLot
```

### Configure Gradle

Create `android/local.properties`:

```properties
sdk.dir=/path/to/android/sdk
ndk.dir=/path/to/android/ndk
```

**macOS Example:**
```properties
sdk.dir=/Users/username/Library/Android/sdk
ndk.dir=/Users/username/Library/Android/sdk/ndk/25.1.8387766
```

**Linux Example:**
```properties
sdk.dir=/home/username/Android/sdk
ndk.dir=/home/username/Android/ndk/android-ndk-r25b
```

**Windows Example:**
```properties
sdk.dir=C:\\Users\\username\\AppData\\Local\\Android\\sdk
ndk.dir=C:\\Users\\username\\AppData\\Local\\Android\\sdk\\ndk\\25.1.8387766
```

---

## Building Locally

### Basic Build

```bash
cd android

# Make gradlew executable (Linux/macOS)
chmod +x gradlew

# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Build App Bundle
./gradlew bundleRelease
```

### Build Output

**Debug APK:**
```
android/app/build/outputs/apk/debug/app-debug.apk
```

**Release APK:**
```
android/app/build/outputs/apk/release/app-release.apk
```

**App Bundle:**
```
android/app/build/outputs/bundle/release/app-release.aab
```

---

## Advanced Build Options

### Build with Custom Options

```bash
# Build with specific build type
./gradlew assembleDebug

# Build with specific flavor
./gradlew assembleDebugFlavor1

# Build with specific task
./gradlew clean assembleDebug

# Build with parallel execution
./gradlew assembleDebug --parallel

# Build with build cache
./gradlew assembleDebug --build-cache

# Build with increased heap size
./gradlew assembleDebug -Dorg.gradle.jvmargs="-Xmx4g"
```

### Performance Optimization

```bash
# Enable daemon for faster builds
./gradlew --daemon assembleDebug

# Stop daemon
./gradlew --stop

# Build with fewer threads
./gradlew assembleDebug -Dorg.gradle.workers.max=2

# Build with parallel compilation
./gradlew assembleDebug --parallel
```

---

## Testing

### Run Unit Tests

```bash
cd android

# Run all unit tests
./gradlew test

# Run specific test class
./gradlew test --tests com.aypilot.nativeapp.VoiceEngineTest

# Run with coverage
./gradlew testDebugUnitTest --coverage
```

### Run Instrumented Tests (on Emulator/Device)

```bash
# Connect Android device or start emulator first

# Run all instrumented tests
./gradlew connectedAndroidTest

# Run specific test class
./gradlew connectedAndroidTest --tests com.aypilot.nativeapp.VoiceEngineInstrumentedTest

# Run with specific device
./gradlew connectedAndroidTest -Pandroid.testInstrumentationRunnerArguments.notAnnotation=androidx.test.filters.FlakyTest
```

### Generate Test Reports

```bash
# Generate unit test report
./gradlew testDebugUnitTest

# View report
open android/app/build/reports/tests/testDebugUnitTest/index.html

# Generate coverage report
./gradlew testDebugUnitTestCoverage
```

---

## Troubleshooting

### Issue: Gradle Daemon Crashes

**Error:** `Gradle daemon has crashed`

**Solution:**
```bash
./gradlew --stop
./gradlew clean
./gradlew assembleDebug
```

### Issue: Out of Memory

**Error:** `java.lang.OutOfMemoryError: GC overhead limit exceeded`

**Solution:**
```bash
# Increase heap size
export GRADLE_OPTS="-Xmx4g"
./gradlew assembleDebug
```

### Issue: NDK Not Found

**Error:** `NDK not found`

**Solution:**
1. Verify NDK is installed: `ls $ANDROID_NDK_HOME`
2. Check `local.properties` has correct NDK path
3. Run: `./gradlew clean`

### Issue: SDK Not Found

**Error:** `Android SDK not found`

**Solution:**
1. Verify SDK is installed: `ls $ANDROID_SDK_ROOT/platforms/`
2. Check `local.properties` has correct SDK path
3. Update SDK: `$ANDROID_SDK_ROOT/tools/bin/sdkmanager --update`

### Issue: JNI Compilation Fails

**Error:** `error: jni.h: No such file or directory`

**Solution:**
1. Verify NDK is installed
2. Check NDK version (should be r25b or later)
3. Clean and rebuild: `./gradlew clean assembleDebug`

### Issue: Build Hangs

**Error:** Build process hangs indefinitely

**Solution:**
```bash
# Kill gradle daemon
./gradlew --stop

# Rebuild with verbose output
./gradlew assembleDebug --info

# Check for network issues
ping google.com
```

---

## Debugging

### Enable Verbose Output

```bash
# Show all build steps
./gradlew assembleDebug --info

# Show even more details
./gradlew assembleDebug --debug

# Show task dependencies
./gradlew assembleDebug --dry-run
```

### Profile Build Performance

```bash
# Generate build profile
./gradlew assembleDebug --profile

# View profile
open android/app/build/reports/profile/profile-2026-07-29-13-30-00.html
```

### Check Dependencies

```bash
# Show dependency tree
./gradlew dependencies

# Show specific configuration
./gradlew dependencies --configuration debugRuntimeClasspath

# Check for conflicts
./gradlew dependencyInsight --dependency com.android.support:appcompat-v7
```

---

## Continuous Integration

### Local CI Simulation

```bash
#!/bin/bash
# simulate-ci.sh

set -e

echo "=== AY Pilot Native - Local CI Simulation ==="
echo ""

cd android

echo "[1/5] Cleaning..."
./gradlew clean

echo "[2/5] Building debug APK..."
./gradlew assembleDebug

echo "[3/5] Running unit tests..."
./gradlew test

echo "[4/5] Building release APK..."
./gradlew assembleRelease

echo "[5/5] Building App Bundle..."
./gradlew bundleRelease

echo ""
echo "✅ All builds successful!"
echo ""
echo "Artifacts:"
echo "  Debug APK: app/build/outputs/apk/debug/app-debug.apk"
echo "  Release APK: app/build/outputs/apk/release/app-release.apk"
echo "  App Bundle: app/build/outputs/bundle/release/app-release.aab"
```

Run simulation:
```bash
chmod +x simulate-ci.sh
./simulate-ci.sh
```

---

## Best Practices

### Development Workflow

1. **Use debug builds** for development
2. **Test on emulator first**, then device
3. **Run unit tests** before committing
4. **Use incremental builds** for faster iteration
5. **Clean periodically** to avoid stale artifacts

### Performance Tips

1. **Enable Gradle daemon** for faster builds
2. **Use build cache** to skip unchanged tasks
3. **Increase heap size** if building large projects
4. **Use parallel compilation** for multi-core systems
5. **Disable unnecessary features** in debug builds

### Security

1. **Never commit signing keys** to repository
2. **Use environment variables** for sensitive data
3. **Sign release builds** before distribution
4. **Verify APK signatures** before installation
5. **Keep dependencies updated** for security patches

---

## Resources

- **Android Gradle Plugin:** https://developer.android.com/studio/build
- **Gradle Documentation:** https://gradle.org/documentation/
- **Android NDK Guide:** https://developer.android.com/ndk/guides
- **Gradle Performance:** https://gradle.org/performance/

---

## Quick Reference

```bash
# Clean build
./gradlew clean assembleDebug

# Build with verbose output
./gradlew assembleDebug --info

# Run tests
./gradlew test

# Build release APK
./gradlew assembleRelease

# Build App Bundle
./gradlew bundleRelease

# Check dependencies
./gradlew dependencies

# Profile build
./gradlew assembleDebug --profile

# Stop daemon
./gradlew --stop
```

---

**Last Updated:** 2026-07-29  
**Status:** Production Ready
