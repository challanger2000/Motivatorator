#include "MotivatoratorProcessor.h"
#include "PhraseBank.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <algorithm>
#include <cstring>

namespace Steinberg::Vst {

using namespace MotivatoratorPhrases;

namespace {
constexpr int kModeMotivator = 0;
constexpr int kModeDemotivator = 1;

inline int normalizedToIndex(ParamValue value, int count) {
    if (count <= 1) return 0;
    return std::clamp(static_cast<int>(value * static_cast<double>(count - 1) + 0.5), 0, count - 1);
}

inline ParamValue indexToNormalized(int index, int count) {
    if (count <= 1) return 0.0;
    return static_cast<ParamValue>(index) / static_cast<ParamValue>(count - 1);
}
} // namespace

MotivatoratorProcessor::MotivatoratorProcessor() {
    setControllerClass(MotivatoratorControllerUID);
}

tresult PLUGIN_API MotivatoratorProcessor::initialize(FUnknown* context) {
    const tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorProcessor::setupProcessing(ProcessSetup& setup) {
    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 44100.0;
    resetIntervalCounter();
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API MotivatoratorProcessor::canProcessSampleSize(int32 s) {
    return (s == kSample32 || s == kSample64) ? kResultTrue : kResultFalse;
}

void MotivatoratorProcessor::handleParameters(ProcessData& data) {
    if (!data.inputParameterChanges) return;

    const int32 count = data.inputParameterChanges->getParameterCount();
    for (int32 i = 0; i < count; ++i) {
        auto* queue = data.inputParameterChanges->getParameterData(i);
        if (!queue || queue->getPointCount() <= 0) continue;

        int32 sampleOffset = 0;
        ParamValue value = 0.0;
        if (queue->getPoint(queue->getPointCount() - 1, sampleOffset, value) != kResultTrue) continue;

        switch (queue->getParameterId()) {
            case kModeId: {
                const int newMode = normalizedToIndex(value, 3);
                if (newMode != mode_) {
                    mode_ = newMode;
                    chooseNextPhrase();
                    needsPhraseEmit_ = true;
                    resetIntervalCounter();
                }
                break;
            }
            case kLanguageId: {
                const int newLanguage = normalizedToIndex(value, 2);
                if (newLanguage != language_) {
                    language_ = newLanguage;
                    chooseNextPhrase();
                    needsPhraseEmit_ = true;
                }
                break;
            }
            case kIntervalId: {
                const int newInterval = normalizedToIndex(value, 6);
                if (newInterval != interval_) {
                    interval_ = newInterval;
                    resetIntervalCounter();
                }
                break;
            }
            case kMuteId:
                muted_ = value >= 0.5;
                break;
            case kNextId: {
                const bool state = value >= 0.5;
                if (state != nextState_) {
                    nextState_ = state;
                    chooseNextPhrase();
                    needsPhraseEmit_ = true;
                    resetIntervalCounter();
                }
                break;
            }
            default:
                break;
        }
    }
}

int MotivatoratorProcessor::nextDeckIndex(bool motivator) {
    int& pos = motivator ? motivatorPos_ : demotivatorPos_;
    int& start = motivator ? motivatorStart_ : demotivatorStart_;
    int& step = motivator ? motivatorStep_ : demotivatorStep_;
    const int count = motivator ? static_cast<int>(kMotivatorCount) : static_cast<int>(kDemotivatorCount);

    const int result = (start + pos * step) % count;
    ++pos;
    if (pos >= count) {
        pos = 0;
        start = (start + 11) % count;
        step = (step == 5) ? 7 : 5;
    }
    return result;
}

void MotivatoratorProcessor::chooseNextPhrase() {
    if (mode_ == kModeMotivator) {
        phrasePositive_ = true;
    } else if (mode_ == kModeDemotivator) {
        phrasePositive_ = false;
    } else {
        phrasePositive_ = mixedNextPositive_;
        mixedNextPositive_ = !mixedNextPositive_;
    }

    const int localIndex = nextDeckIndex(phrasePositive_);
    const int languageBase = language_ == 0 ? 0 : static_cast<int>(kPhraseCount);
    const int toneBase = phrasePositive_ ? 0 : static_cast<int>(kMotivatorCount);
    currentPhraseGlobal_ = languageBase + toneBase + localIndex;
}

void MotivatoratorProcessor::emitPhrase(ProcessData& data) {
    if (!data.outputParameterChanges) return;

    int32 queueIndex = 0;
    if (auto* phraseQueue = data.outputParameterChanges->addParameterData(kPhraseId, queueIndex)) {
        int32 pointIndex = 0;
        phraseQueue->addPoint(0, indexToNormalized(currentPhraseGlobal_, static_cast<int>(kPhraseCount * 2)), pointIndex);
    }

    queueIndex = 0;
    if (auto* toneQueue = data.outputParameterChanges->addParameterData(kPhraseToneId, queueIndex)) {
        int32 pointIndex = 0;
        toneQueue->addPoint(0, phrasePositive_ ? 0.0 : 1.0, pointIndex);
    }
}

void MotivatoratorProcessor::resetIntervalCounter() {
    double seconds = 60.0;
    switch (interval_) {
        case 0: seconds = 15.0; break;
        case 1: seconds = 30.0; break;
        case 2: seconds = 60.0; break;
        case 3: seconds = 120.0; break;
        case 4: seconds = 300.0; break;
        case 5:
        default:
            intervalRng_ ^= intervalRng_ << 13;
            intervalRng_ ^= intervalRng_ >> 17;
            intervalRng_ ^= intervalRng_ << 5;
            seconds = 20.0 + static_cast<double>(intervalRng_ % 101u);
            break;
    }
    samplesUntilNext_ = static_cast<int64>(seconds * sampleRate_);
}

tresult PLUGIN_API MotivatoratorProcessor::process(ProcessData& data) {
    handleParameters(data);

    if (data.numInputs > 0 && data.numOutputs > 0 && data.numSamples > 0) {
        const auto& input = data.inputs[0];
        auto& output = data.outputs[0];
        const int32 channels = input.numChannels < output.numChannels ? input.numChannels : output.numChannels;

        if (data.symbolicSampleSize == kSample32) {
            for (int32 ch = 0; ch < channels; ++ch) {
                if (input.channelBuffers32[ch] && output.channelBuffers32[ch] && input.channelBuffers32[ch] != output.channelBuffers32[ch]) {
                    std::memcpy(output.channelBuffers32[ch], input.channelBuffers32[ch], static_cast<size_t>(data.numSamples) * sizeof(Sample32));
                }
            }
        } else if (data.symbolicSampleSize == kSample64) {
            for (int32 ch = 0; ch < channels; ++ch) {
                if (input.channelBuffers64[ch] && output.channelBuffers64[ch] && input.channelBuffers64[ch] != output.channelBuffers64[ch]) {
                    std::memcpy(output.channelBuffers64[ch], input.channelBuffers64[ch], static_cast<size_t>(data.numSamples) * sizeof(Sample64));
                }
            }
        }
        output.silenceFlags = input.silenceFlags;
    }

    if (needsPhraseEmit_) {
        if (motivatorPos_ == 0 && demotivatorPos_ == 0)
            chooseNextPhrase();
        emitPhrase(data);
        needsPhraseEmit_ = false;
    }

    if (!muted_ && data.numSamples > 0) {
        samplesUntilNext_ -= data.numSamples;
        if (samplesUntilNext_ <= 0) {
            chooseNextPhrase();
            emitPhrase(data);
            resetIntervalCounter();
        }
    }

    return kResultOk;
}

tresult PLUGIN_API MotivatoratorController::initialize(FUnknown* context) {
    auto result = EditControllerEx1::initialize(context);
    if (result != kResultOk) return result;

    auto* mode = new StringListParameter(STR16("Mode"), kModeId);
    mode->appendString(STR16("MOTIVATOR"));
    mode->appendString(STR16("DEMOTIVATOR"));
    mode->appendString(STR16("MIXED"));
    parameters.addParameter(mode);

    parameters.addParameter(STR16("Next"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, kNextId);
    parameters.addParameter(STR16("Mute Me"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, kMuteId);
    parameters.addParameter(STR16("Options"), nullptr, 1, 0.0, 0, kOptionsId);

    auto* language = new StringListParameter(STR16("Language"), kLanguageId);
    language->appendString(STR16("Deutsch"));
    language->appendString(STR16("English"));
    parameters.addParameter(language);

    auto* interval = new StringListParameter(STR16("Interval"), kIntervalId);
    interval->appendString(STR16("15 sec"));
    interval->appendString(STR16("30 sec"));
    interval->appendString(STR16("1 min"));
    interval->appendString(STR16("2 min"));
    interval->appendString(STR16("5 min"));
    interval->appendString(STR16("Random"));
    parameters.addParameter(interval);
    interval->setNormalized(0.4);

    auto* character = new StringListParameter(STR16("Character"), kCharacterId);
    character->appendString(STR16("GNOMI"));
    character->appendString(STR16("ROCKY"));
    character->appendString(STR16("D.O.M."));
    parameters.addParameter(character);

    auto* phrase = new StringListParameter(STR16("Phrase"), kPhraseId);
    for (const auto& p : kMotivator) phrase->appendString(p.de);
    for (const auto& p : kDemotivator) phrase->appendString(p.de);
    for (const auto& p : kMotivator) phrase->appendString(p.en);
    for (const auto& p : kDemotivator) phrase->appendString(p.en);
    parameters.addParameter(phrase);

    auto* tone = new StringListParameter(STR16("Phrase Tone"), kPhraseToneId);
    tone->appendString(STR16("POSITIVE"));
    tone->appendString(STR16("NEGATIVE"));
    parameters.addParameter(tone);

    return kResultOk;
}

IPlugView* PLUGIN_API MotivatoratorController::createView(FIDString name) {
    if (name && std::strcmp(name, ViewType::kEditor) == 0)
        return new VSTGUI::VST3Editor(this, "view", "Motivatorator.uidesc");
    return nullptr;
}

} // namespace Steinberg::Vst
