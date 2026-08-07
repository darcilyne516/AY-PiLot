# Android Build Quick Start

## Prerequisites

1. **Java 11+**
   ```bash
   java -version
   ```

2. **Android SDK**
   ```bash
   export ANDROID_SDK_ROOT=/path/to/android/sdk
   ```

3. **Android NDK (r25b+)**
   ```bash
   export ANDROID_NDK_HOME=/path/to/android/ndk
   ```

## Quick Build

### Option 1: Using Test Script (Recommended)

```bash
cd /path/to/AY-PiLot

# Build debug APK
./scripts/test-android-build.sh debug

# Build release APK and bundle
./scripts/test-android-build.sh release

# Build everything
./scripts/test-android-build.sh all
```

### Option 2: Manual Gradle Commands

```bash
cd android

# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Build App Bundle
./gradlew bundleRelease

# Run tests
./gradlew test
```

## Output Locations

- **Debug APK:** `android/app/build/outputs/apk/debug/app-debug.apk`
- **Release APK:** `android/app/build/outputs/apk/release/app-release.apk`
- **App Bundle:** `android/app/build/outputs/bundle/release/app-release.aab`

## Testing on Device/Emulator

```bash
# Install debug APK
adb install android/app/build/outputs/apk/debug/app-debug.apk

# Run app
adb shell am start -n com.aypilot.nativeapp/.MainActivity
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Java not found | Install JDK 11+ and set JAVA_HOME |
| Android SDK not found | Set ANDROID_SDK_ROOT environment variable |
| Android NDK not found | Set ANDROID_NDK_HOME environment variable |
| Build hangs | Run `./gradlew --stop` and retry |
| Out of memory | Set `export GRADLE_OPTS="-Xmx4g"` |

## Next Steps

1. ✅ Build locally using test script
2. ✅ Test on Android emulator or device
3. ✅ Push to GitHub
4. ✅ GitHub Actions will build automatically

---

**For detailed guide:** See `docs/ANDROID-LOCAL-BUILD.md`
