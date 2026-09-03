#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "VoicePrototype.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace Steinberg::Vst {

static const FUID MotivatoratorProcessorUID(0x4D4F5449, 0x5641544F, 0x52465830, 0x30303031);
static const FUID MotivatoratorControllerUID(0x4D4F5449, 0x5641544F, 0x52435430, 0x30303031);

enum MotivatoratorParams : ParamID {
    kModeId = 100,
    // 101 intentionally remains unused to avoid reusing the legacy NEXT parameter ID.
    kNextId = 101,
    kMuteId = 102,
    kOptionsId = 103,
    kLanguageId = 104,
    kIntervalId = 105,
    kCharacterId = 106,
    kPhraseId = 107,
    kPhraseToneId = 108,
    kMessageSoundId = 109,
    kPingVolumeId = 110,
    kVoiceEnabledId = 111,
    kVoiceVolumeId = 112
};

class MotivatoratorProcessor final : public AudioEffect {
public:
    MotivatoratorProcessor();
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) SMTG_OVERRIDE;
    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
    tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
    static FUnknown* createInstance(void*) { return static_cast<IAudioProcessor*>(new MotivatoratorProcessor()); }

private:
    void handleParameters(ProcessData& data);
    void chooseNextPhrase();
    void emitPhrase(ProcessData& data);
    void resetIntervalCounter();
    void triggerPing();
    void mixPing(ProcessData& data);
    void mixVoicePrototype(ProcessData& data);
    void requestVoicePrototype();
    void reshuffleDeck(bool motivator) noexcept;
    int nextDeckIndex(bool motivator) noexcept;
    static uint32_t nextRandom(uint32_t& state) noexcept;

    static uint32_t startupSeed() {
        static std::atomic<uint32_t> sequence {0};
        const auto ticks = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        const uint32_t n = sequence.fetch_add(1, std::memory_order_relaxed) + 1u;
        return static_cast<uint32_t>(ticks ^ (ticks >> 32)) ^ (0x9E3779B9u * n);
    }

    double sampleRate_ {44100.0};
    int mode_ {0};
    int language_ {0};
    int interval_ {2};
    int character_ {0};
    bool muted_ {false};
    bool phrasePositive_ {true};
    bool needsPhraseEmit_ {true};
    bool messageSound_ {true};
    double pingVolume_ {0.5};
    bool voiceEnabled_ {true};
    double voiceVolume_ {0.5};
    int64 pingSamplesRemaining_ {0};
    int64 pingSamplesTotal_ {0};
    double pingPhase_ {0.0};
    uint32_t mixedRandomState_ {startupSeed() | 1u};
    uint32_t motivatorShuffleState_ {startupSeed() | 1u};
    uint32_t demotivatorShuffleState_ {startupSeed() | 1u};
    std::array<uint16_t, 500> motivatorDeck_ {};
    std::array<uint16_t, 500> demotivatorDeck_ {};
    int motivatorPos_ {0};
    int demotivatorPos_ {0};
    int currentPhraseGlobal_ {0};
    int64 samplesUntilNext_ {0};
    VoicePrototype voicePrototype_;
};

class MotivatoratorController final : public EditControllerEx1 {
public:
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
    static FUnknown* createInstance(void*) { return static_cast<IEditController*>(new MotivatoratorController()); }
};

} // namespace Steinberg::Vst
