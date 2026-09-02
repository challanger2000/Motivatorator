#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Steinberg::Vst {

// Experimental Windows-only TTS bridge. Synthesis is performed on a worker
// thread; the realtime audio thread only consumes an already prepared buffer.
class VoicePrototype {
public:
    VoicePrototype();
    ~VoicePrototype();

    VoicePrototype(const VoicePrototype&) = delete;
    VoicePrototype& operator=(const VoicePrototype&) = delete;

    void setSampleRate(double sampleRate);
    void request(const std::u16string& text);
    float nextSample() noexcept;
    void resetPlayback() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Steinberg::Vst
