#include "MotivatoratorProcessor.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <cstring>

namespace Steinberg::Vst {

MotivatoratorProcessor::MotivatoratorProcessor() { setControllerClass(MotivatoratorControllerUID); }

tresult PLUGIN_API MotivatoratorProcessor::initialize(FUnknown* context) {
    const tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorProcessor::canProcessSampleSize(int32 s) {
    return (s == kSample32 || s == kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API MotivatoratorProcessor::process(ProcessData& data) {
    if (data.numInputs < 1 || data.numOutputs < 1 || data.numSamples <= 0) return kResultOk;
    const auto& input = data.inputs[0]; auto& output = data.outputs[0];
    const int32 channels = input.numChannels < output.numChannels ? input.numChannels : output.numChannels;
    if (data.symbolicSampleSize == kSample32) {
        for (int32 ch=0; ch<channels; ++ch) if (input.channelBuffers32[ch] && output.channelBuffers32[ch] && input.channelBuffers32[ch] != output.channelBuffers32[ch])
            std::memcpy(output.channelBuffers32[ch], input.channelBuffers32[ch], static_cast<size_t>(data.numSamples)*sizeof(Sample32));
    } else if (data.symbolicSampleSize == kSample64) {
        for (int32 ch=0; ch<channels; ++ch) if (input.channelBuffers64[ch] && output.channelBuffers64[ch] && input.channelBuffers64[ch] != output.channelBuffers64[ch])
            std::memcpy(output.channelBuffers64[ch], input.channelBuffers64[ch], static_cast<size_t>(data.numSamples)*sizeof(Sample64));
    }
    output.silenceFlags = input.silenceFlags;
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorController::initialize(FUnknown* context) {
    auto result = EditControllerEx1::initialize(context);
    if (result != kResultOk) return result;
    parameters.addParameter(STR16("Mode"), nullptr, 2, 0.0, ParameterInfo::kCanAutomate, kModeId);
    parameters.addParameter(STR16("Next"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, kNextId);
    parameters.addParameter(STR16("Mute Me"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, kMuteId);
    parameters.addParameter(STR16("Options"), nullptr, 1, 0.0, 0, kOptionsId);
    parameters.addParameter(STR16("Language"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, kLanguageId);
    parameters.addParameter(STR16("Interval"), STR16("s"), 5, 0.4, ParameterInfo::kCanAutomate, kIntervalId);
    return kResultOk;
}

IPlugView* PLUGIN_API MotivatoratorController::createView(FIDString name) {
    if (name && std::strcmp(name, ViewType::kEditor) == 0)
        return new VSTGUI::VST3Editor(this, "view", "Motivatorator.uidesc");
    return nullptr;
}

} // namespace Steinberg::Vst
