#pragma once

#include <memory>

namespace Steinberg::Vst {

// Windows-only TTS bridge. The realtime thread publishes only compact numeric
// requests and consumes already prepared audio. Text lookup, TTS synthesis,
// allocation and DSP all stay on the worker thread.
class VoicePrototype {
public:
    VoicePrototype();
    ~VoicePrototype();

    VoicePrototype(const VoicePrototype&) = delete;
    VoicePrototype& operator=(const VoicePrototype&) = delete;

    void setSampleRate(double sampleRate);
    void request(int phraseGlobal, int character) noexcept;
    float nextSample() noexcept;
    void resetPlayback() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Steinberg::Vst
