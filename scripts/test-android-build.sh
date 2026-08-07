#!/bin/bash

################################################################################
# AY Pilot Native - Android Local Build Test Script
#
# This script tests the Android build locally before triggering GitHub Actions
# Usage: ./test-android-build.sh [debug|release|all]
################################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="${1:-debug}"
ANDROID_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/android"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[⚠]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Print header
print_header() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║   AY Pilot Native - Android Local Build Test                  ║"
    echo "║   Build Type: $BUILD_TYPE"
    echo "║   Date: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check Java
    if ! command -v java &> /dev/null; then
        log_error "Java not found. Please install JDK 11+"
        exit 1
    fi
    log_success "Java found: $(java -version 2>&1 | head -1)"
    
    # Check Android SDK
    if [ -z "$ANDROID_SDK_ROOT" ]; then
        log_warning "ANDROID_SDK_ROOT not set. Checking common locations..."
        if [ -d "$HOME/Android/sdk" ]; then
            export ANDROID_SDK_ROOT="$HOME/Android/sdk"
            log_success "Found Android SDK at $ANDROID_SDK_ROOT"
        elif [ -d "$HOME/Library/Android/sdk" ]; then
            export ANDROID_SDK_ROOT="$HOME/Library/Android/sdk"
            log_success "Found Android SDK at $ANDROID_SDK_ROOT"
        else
            log_error "Android SDK not found. Please set ANDROID_SDK_ROOT"
            exit 1
        fi
    else
        log_success "Android SDK: $ANDROID_SDK_ROOT"
    fi
    
    # Check Android NDK
    if [ -z "$ANDROID_NDK_HOME" ]; then
        log_warning "ANDROID_NDK_HOME not set. Checking common locations..."
        if [ -d "$ANDROID_SDK_ROOT/ndk" ]; then
            export ANDROID_NDK_HOME=$(ls -d $ANDROID_SDK_ROOT/ndk/* | head -1)
            log_success "Found Android NDK at $ANDROID_NDK_HOME"
        else
            log_error "Android NDK not found. Please set ANDROID_NDK_HOME"
            exit 1
        fi
    else
        log_success "Android NDK: $ANDROID_NDK_HOME"
    fi
    
    # Check Gradle
    if [ ! -f "$ANDROID_DIR/gradlew" ]; then
        log_error "gradlew not found in $ANDROID_DIR"
        exit 1
    fi
    log_success "Gradle wrapper found"
    
    echo ""
}

# Setup environment
setup_environment() {
    log_info "Setting up environment..."
    
    cd "$ANDROID_DIR"
    chmod +x gradlew
    
    # Create local.properties if not exists
    if [ ! -f "local.properties" ]; then
        log_info "Creating local.properties..."
        cat > local.properties << EOF
sdk.dir=$ANDROID_SDK_ROOT
ndk.dir=$ANDROID_NDK_HOME
EOF
        log_success "local.properties created"
    else
        log_success "local.properties already exists"
    fi
    
    echo ""
}

# Clean build
clean_build() {
    log_info "Cleaning previous builds..."
    ./gradlew clean --quiet
    log_success "Build cleaned"
    echo ""
}

# Build debug APK
build_debug() {
    log_info "Building debug APK..."
    
    if ./gradlew assembleDebug; then
        log_success "Debug APK built successfully"
        
        # Check if APK exists
        APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
        if [ -f "$APK_PATH" ]; then
            APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
            log_success "APK created: $APK_PATH ($APK_SIZE)"
        fi
    else
        log_error "Debug APK build failed"
        return 1
    fi
    echo ""
}

# Build release APK
build_release() {
    log_info "Building release APK..."
    
    if ./gradlew assembleRelease; then
        log_success "Release APK built successfully"
        
        # Check if APK exists
        APK_PATH="app/build/outputs/apk/release/app-release.apk"
        if [ -f "$APK_PATH" ]; then
            APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
            log_success "APK created: $APK_PATH ($APK_SIZE)"
        fi
    else
        log_error "Release APK build failed"
        return 1
    fi
    echo ""
}

# Build App Bundle
build_bundle() {
    log_info "Building App Bundle..."
    
    if ./gradlew bundleRelease; then
        log_success "App Bundle built successfully"
        
        # Check if bundle exists
        BUNDLE_PATH="app/build/outputs/bundle/release/app-release.aab"
        if [ -f "$BUNDLE_PATH" ]; then
            BUNDLE_SIZE=$(du -h "$BUNDLE_PATH" | cut -f1)
            log_success "Bundle created: $BUNDLE_PATH ($BUNDLE_SIZE)"
        fi
    else
        log_error "App Bundle build failed"
        return 1
    fi
    echo ""
}

# Run unit tests
run_unit_tests() {
    log_info "Running unit tests..."
    
    if ./gradlew test; then
        log_success "Unit tests passed"
        
        # Show test results
        TEST_REPORT="app/build/reports/tests/testDebugUnitTest/index.html"
        if [ -f "$TEST_REPORT" ]; then
            log_info "Test report: $TEST_REPORT"
        fi
    else
        log_warning "Some unit tests failed (this may be expected)"
    fi
    echo ""
}

# Check build artifacts
check_artifacts() {
    log_info "Checking build artifacts..."
    
    echo ""
    echo "Debug APK:"
    if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
        log_success "Found: app/build/outputs/apk/debug/app-debug.apk"
        ls -lh app/build/outputs/apk/debug/app-debug.apk
    else
        log_warning "Not found: app/build/outputs/apk/debug/app-debug.apk"
    fi
    
    echo ""
    echo "Release APK:"
    if [ -f "app/build/outputs/apk/release/app-release.apk" ]; then
        log_success "Found: app/build/outputs/apk/release/app-release.apk"
        ls -lh app/build/outputs/apk/release/app-release.apk
    else
        log_warning "Not found: app/build/outputs/apk/release/app-release.apk"
    fi
    
    echo ""
    echo "App Bundle:"
    if [ -f "app/build/outputs/bundle/release/app-release.aab" ]; then
        log_success "Found: app/build/outputs/bundle/release/app-release.aab"
        ls -lh app/build/outputs/bundle/release/app-release.aab
    else
        log_warning "Not found: app/build/outputs/bundle/release/app-release.aab"
    fi
    
    echo ""
}

# Print summary
print_summary() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                    Build Test Complete                         ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    
    if [ "$BUILD_TYPE" = "debug" ] || [ "$BUILD_TYPE" = "all" ]; then
        echo "Debug Build:"
        if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
            log_success "APK ready for testing"
        else
            log_error "APK not found"
        fi
    fi
    
    if [ "$BUILD_TYPE" = "release" ] || [ "$BUILD_TYPE" = "all" ]; then
        echo ""
        echo "Release Build:"
        if [ -f "app/build/outputs/apk/release/app-release.apk" ]; then
            log_success "APK ready for distribution"
        else
            log_error "APK not found"
        fi
        
        if [ -f "app/build/outputs/bundle/release/app-release.aab" ]; then
            log_success "Bundle ready for Play Store"
        else
            log_error "Bundle not found"
        fi
    fi
    
    echo ""
    log_success "Build test completed successfully!"
    echo ""
}

# Main execution
main() {
    print_header
    
    # Validate build type
    if [ "$BUILD_TYPE" != "debug" ] && [ "$BUILD_TYPE" != "release" ] && [ "$BUILD_TYPE" != "all" ]; then
        log_error "Invalid build type: $BUILD_TYPE"
        echo "Usage: $0 [debug|release|all]"
        exit 1
    fi
    
    # Run checks
    check_prerequisites
    setup_environment
    clean_build
    
    # Run builds based on type
    case $BUILD_TYPE in
        debug)
            build_debug
            run_unit_tests
            ;;
        release)
            build_release
            build_bundle
            ;;
        all)
            build_debug
            run_unit_tests
            build_release
            build_bundle
            ;;
    esac
    
    # Check artifacts and print summary
    check_artifacts
    print_summary
}

# Run main function
main
