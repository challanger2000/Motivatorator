#include "MotivatoratorProcessor.h"
#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF("Motivatorator", "https://github.com/challanger2000/", "")

DEF_CLASS2(INLINE_UID_FROM_FUID(MotivatoratorProcessorUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "Motivatorator",
           Vst::kDistributable,
           Vst::PlugType::kFx,
           "0.1.0",
           kVstVersionString,
           MotivatoratorProcessor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(MotivatoratorControllerUID),
           PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           "MotivatoratorController",
           0,
           "",
           "0.1.0",
           kVstVersionString,
           MotivatoratorController::createInstance)

END_FACTORY
