#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <cstdint>

namespace Steinberg::Vst {

static const FUID MotivatoratorProcessorUID(0x4D4F5449, 0x5641544F, 0x52465830, 0x30303031);
static const FUID MotivatoratorControllerUID(0x4D4F5449, 0x5641544F, 0x52435430, 0x30303031);

enum MotivatoratorParams : ParamID {
    kModeId = 100,
    kNextId = 101,
    kMuteId = 102,
    kOptionsId = 103,
    kLanguageId = 104,
    kIntervalId = 105,
    kCharacterId = 106,
    kPhraseId = 107,
    kPhraseToneId = 108
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
    int nextDeckIndex(bool motivator);

    double sampleRate_ {44100.0};
    int mode_ {0};
    int language_ {0};
    int interval_ {2};
    int character_ {0};
    bool muted_ {false};
    bool nextState_ {false};
    bool mixedNextPositive_ {true};
    bool phrasePositive_ {true};
    bool needsPhraseEmit_ {true};
    int motivatorPos_ {0};
    int demotivatorPos_ {0};
    int motivatorStart_ {0};
    int demotivatorStart_ {9};
    int motivatorStep_ {5};
    int demotivatorStep_ {7};
    int currentPhraseGlobal_ {0};
    int64 samplesUntilNext_ {0};
    uint32_t intervalRng_ {0x4D4F5449u};
};

class MotivatoratorController final : public EditControllerEx1 {
public:
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
    static FUnknown* createInstance(void*) { return static_cast<IEditController*>(new MotivatoratorController()); }
};

} // namespace Steinberg::Vst
