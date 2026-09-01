#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg::Vst {

static const FUID MotivatoratorProcessorUID(0x4D4F5449, 0x5641544F, 0x52465830, 0x30303031);
static const FUID MotivatoratorControllerUID(0x4D4F5449, 0x5641544F, 0x52435430, 0x30303031);

enum MotivatoratorParams : ParamID {
    kModeId = 100,
    kNextId = 101,
    kMuteId = 102,
    kOptionsId = 103,
    kLanguageId = 104,
    kIntervalId = 105
};

class MotivatoratorProcessor final : public AudioEffect {
public:
    MotivatoratorProcessor();
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
    static FUnknown* createInstance(void*) { return static_cast<IAudioProcessor*>(new MotivatoratorProcessor()); }
};

class MotivatoratorController final : public EditControllerEx1 {
public:
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
    static FUnknown* createInstance(void*) { return static_cast<IEditController*>(new MotivatoratorController()); }
};

} // namespace Steinberg::Vst
