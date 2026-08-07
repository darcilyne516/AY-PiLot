#include "ay_pilot_engine.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

int main() {
    ay_pilot::VoiceEngine engine(48000, 512);
    
    // Generate a simple 100Hz sine wave as a test signal
    std::vector<int16_t> test_audio(48000, 0); // 1 second
    for (int i = 0; i < 48000; ++i) {
        test_audio[i] = static_cast<int16_t>(10000.0 * sin(2.0 * M_PI * 100.0 * i / 48000.0));
    }

    ay_pilot::VoiceProfile profile = engine.analyze_voice(test_audio);

    std::cout << "Analyzed F0: " << profile.f0_mean << " Hz" << std::endl;
    
    // Expect F0 to be close to 100Hz
    if (std::abs(profile.f0_mean - 100.0f) < 15.0f) {
        std::cout << "F0 analysis test PASSED" << std::endl;
    } else {
        std::cout << "F0 analysis test FAILED" << std::endl;
        return 1;
    }

    std::cout << "LPC coefficients count: " << profile.formants.size() << std::endl;
    assert(profile.formants.size() == 15); // order 14 + 1

    return 0;
}
