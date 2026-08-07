# AY Pilot Native — Build Instructions

Complete step-by-step guide for building AY Pilot Native for all platforms.

## Prerequisites

### All Platforms
- Git
- CMake 3.10 or higher
- C++17 compatible compiler

### Android
- Android Studio 2021.1 or higher
- Android NDK 23.1 or higher
- Android SDK (API level 29+)
- Gradle 7.0 or higher
- JDK 11 or higher

### Desktop
- Node.js 16 or higher
- npm or pnpm
- Python 3.6 or higher
- Platform-specific build tools:
  - **Windows:** Visual Studio 2019+ or MinGW-w64
  - **macOS:** Xcode 12 or higher
  - **Linux:** GCC 9+ or Clang 10+

## Building the Shared Engine

The shared C++ engine is the core of AY Pilot Native. It must be built first.

### Step 1: Clone the Repository

```bash
git clone https://github.com/aypilot/ay-pilot-native.git
cd ay-pilot-native
```

### Step 2: Build with CMake

```bash
cd shared-engine
mkdir build
cd build
cmake ..
make
```

**Output:** `libay_pilot_engine.a` (static library)

### Step 3: Verify the Build

```bash
# List generated files
ls -la
# Expected: CMakeFiles/, Makefile, libay_pilot_engine.a, cmake_install.cmake
```

## Building for Android

### Step 1: Set Up Android Environment

```bash
# Set ANDROID_NDK_HOME (adjust path as needed)
export ANDROID_NDK_HOME=/path/to/android-ndk-r23.1
export ANDROID_SDK_ROOT=/path/to/android-sdk

# Verify installation
$ANDROID_NDK_HOME/ndk-build --version
```

### Step 2: Configure Android Project

```bash
cd android

# Edit local.properties to point to your Android SDK
echo "sdk.dir=/path/to/android-sdk" > local.properties
echo "ndk.dir=/path/to/android-ndk-r23.1" >> local.properties
```

### Step 3: Build APK (Debug)

```bash
./gradlew clean build
```

**Output:** `app/build/outputs/apk/debug/app-debug.apk`

### Step 4: Build Release APK

```bash
./gradlew assembleRelease
```

**Output:** `app/build/outputs/apk/release/app-release-unsigned.apk`

### Step 5: Sign the APK

```bash
# Create a keystore (one-time)
keytool -genkey -v -keystore ~/ay-pilot.keystore \
  -keyalg RSA -keysize 2048 -validity 10000 \
  -alias ay-pilot-key

# Sign the APK
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 \
  -keystore ~/ay-pilot.keystore \
  app/build/outputs/apk/release/app-release-unsigned.apk \
  ay-pilot-key

# Align the APK
zipalign -v 4 app/build/outputs/apk/release/app-release-unsigned.apk \
  app/build/outputs/apk/release/app-release.apk
```

### Step 6: Build App Bundle (for Google Play)

```bash
./gradlew bundleRelease
```

**Output:** `app/build/outputs/bundle/release/app-release.aab`

### Troubleshooting Android Build

**Issue:** `CMake not found`
- **Solution:** Install CMake via Android Studio SDK Manager (Tools → SDK Manager → SDK Tools → CMake)

**Issue:** `NDK not found`
- **Solution:** Set `ANDROID_NDK_HOME` environment variable correctly

**Issue:** `Gradle build fails with symbol not found`
- **Solution:** Ensure JNI bindings in `app/src/main/cpp/ay_pilot_jni.cpp` match Kotlin class names exactly

## Building for Desktop

### Windows (MSVC)

#### Step 1: Install Build Tools

```bash
# Using Visual Studio
# Install C++ workload and CMake tools

# Or using vcpkg for dependencies
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```

#### Step 2: Build the Desktop App

```bash
cd desktop

# Install dependencies
npm install

# Build N-API module
npm run build

# Create installer
npm run package
```

**Output:** `dist/AY Pilot Setup 1.0.0.exe`

### macOS

#### Step 1: Install Xcode Command Line Tools

```bash
xcode-select --install
```

#### Step 2: Build the Desktop App

```bash
cd desktop

# Install dependencies
npm install

# Build N-API module
npm run build

# Create DMG installer
npm run package
```

**Output:** `dist/AY Pilot-1.0.0.dmg`

### Linux (Ubuntu/Debian)

#### Step 1: Install Build Dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  nodejs \
  npm \
  python3 \
  libx11-dev \
  libxext-dev
```

#### Step 2: Build the Desktop App

```bash
cd desktop

# Install dependencies
npm install

# Build N-API module
npm run build

# Create AppImage
npm run package
```

**Output:** `dist/AY Pilot-1.0.0.AppImage`

## Building the Landing Page Website

### Prerequisites
- Node.js 16+
- pnpm (recommended) or npm

### Step 1: Install Dependencies

```bash
cd ../ay-pilot-website
pnpm install
```

### Step 2: Development Server

```bash
pnpm dev
```

**Output:** Local dev server at `http://localhost:3000`

### Step 3: Production Build

```bash
pnpm build
```

**Output:** Optimized static files in `dist/`

### Step 4: Deploy

```bash
# Deploy to Manus (if using Manus platform)
pnpm run deploy

# Or deploy to your own hosting
# Copy dist/ contents to your web server
```

## Running Tests

### Unit Tests (Voice Analysis Engine)

```bash
cd tests

# Compile test
g++ -Iay-pilot-native/shared-engine/include \
    ay-pilot-native/shared-engine/src/ay_pilot_engine.cpp \
    test_analysis.cpp -o test_analysis

# Run test
./test_analysis
```

**Expected Output:**
```
Analyzed F0: 111.989 Hz
F0 analysis test PASSED
LPC coefficients count: 15
```

### Integration Tests (Full Audio Pipeline)

**Android:**
```bash
cd android
./gradlew connectedAndroidTest
```

**Desktop:**
```bash
cd desktop
npm run test
```

## Continuous Integration (CI/CD)

### GitHub Actions Example

Create `.github/workflows/build.yml`:

```yaml
name: Build AY Pilot Native

on: [push, pull_request]

jobs:
  build-shared-engine:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Shared Engine
        run: |
          cd shared-engine
          mkdir build && cd build
          cmake ..
          make

  build-android:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-java@v2
        with:
          java-version: '11'
      - name: Build Android
        run: |
          cd android
          ./gradlew assembleRelease

  build-desktop-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-node@v2
        with:
          node-version: '16'
      - name: Build Desktop (Linux)
        run: |
          cd desktop
          npm install
          npm run build
          npm run package

  build-desktop-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-node@v2
        with:
          node-version: '16'
      - name: Build Desktop (macOS)
        run: |
          cd desktop
          npm install
          npm run build
          npm run package

  build-desktop-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-node@v2
        with:
          node-version: '16'
      - name: Build Desktop (Windows)
        run: |
          cd desktop
          npm install
          npm run build
          npm run package
```

## Release Checklist

Before releasing a new version:

- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Code reviewed and approved
- [ ] Version number updated in all build files
- [ ] Changelog updated
- [ ] Signed APK/AAB created for Android
- [ ] Installers created for all desktop platforms
- [ ] Website updated with new download links
- [ ] Release notes published
- [ ] Announcement posted to community channels

## Troubleshooting

### Common Build Issues

**Issue:** `CMake version 3.10 or higher required`
- **Solution:** Update CMake: `sudo apt-get install cmake` (Linux) or download from cmake.org

**Issue:** `C++17 compiler not found`
- **Solution:** Update compiler: `sudo apt-get install g++` (Linux) or install Xcode (macOS)

**Issue:** `Node.js version too old`
- **Solution:** Update Node.js from nodejs.org or using nvm

**Issue:** `Gradle build timeout`
- **Solution:** Increase timeout in `gradle.properties`: `org.gradle.daemon.performance.enable=true`

## Support

For build issues:
1. Check the troubleshooting section above
2. Review platform-specific documentation
3. Open an issue on GitHub with build logs
4. Join the community Discord for real-time help

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-29
