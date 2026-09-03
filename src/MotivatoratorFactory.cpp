#include "MotivatoratorProcessor.h"
#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF("Motivator", "https://github.com/challanger2000/", "")

DEF_CLASS2(INLINE_UID_FROM_FUID(MotivatoratorProcessorUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "Motivator",
           Vst::kDistributable,
           Vst::PlugType::kFx,
           "0.1.0",
           kVstVersionString,
           MotivatoratorProcessor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(MotivatoratorControllerUID),
           PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           "MotivatorController",
           0,
           "",
           "0.1.0",
           kVstVersionString,
           MotivatoratorController::createInstance)

END_FACTORY
