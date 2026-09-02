#include "MotivatoratorProcessor.h"
#include "MotivatoratorEditor.h"
#include "PhraseBank.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Steinberg::Vst {

using namespace MotivatoratorPhrases;

namespace {
constexpr int kModeMotivator = 0;
constexpr int kModeDemotivator = 1;
constexpr int kModeMixed = 2;
constexpr int32 kStateVersion = 1;
constexpr double kPi = 3.14159265358979323846;

int normalizedToIndex(ParamValue v, int count) {
    return std::clamp(static_cast<int>(v * count), 0, count - 1);
}
}

MotivatoratorProcessor::MotivatoratorProcessor() { setControllerClass(MotivatoratorControllerUID); }

tresult PLUGIN_API MotivatoratorProcessor::initialize(FUnknown* context) {
    auto result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorProcessor::setupProcessing(ProcessSetup& setup) {
    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 44100.0;
    pingSamplesRemaining_ = 0;
    pingSamplesTotal_ = 0;
    pingPhase_ = 0.0;
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API MotivatoratorProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    return symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API MotivatoratorProcessor::getState(IBStream* state) {
    if (!state) return kInvalidArgument;
    IBStreamer stream(state, kLittleEndian);
    stream.writeInt32(kStateVersion); stream.writeInt32(mode_); stream.writeInt32(language_); stream.writeInt32(interval_);
    stream.writeInt32(character_); stream.writeInt32(muted_ ? 1 : 0); stream.writeInt32(mixedNextPositive_ ? 1 : 0);
    stream.writeInt32(phrasePositive_ ? 1 : 0); stream.writeInt32(motivatorPos_); stream.writeInt32(demotivatorPos_);
    stream.writeInt32(motivatorStart_); stream.writeInt32(demotivatorStart_); stream.writeInt32(motivatorStep_); stream.writeInt32(demotivatorStep_);
    stream.writeInt32(currentPhraseGlobal_);
    stream.writeInt32(messageSound_ ? 1 : 0);
    stream.writeDouble(pingVolume_);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorProcessor::setState(IBStream* state) {
    if (!state) return kInvalidArgument;
    IBStreamer stream(state, kLittleEndian); int32 version=0, value=0;
    if (!stream.readInt32(version) || version != kStateVersion) return kResultFalse;
    if (!stream.readInt32(value)) return kResultFalse; mode_=std::clamp(value,0,2);
    if (!stream.readInt32(value)) return kResultFalse; language_=std::clamp(value,0,1);
    if (!stream.readInt32(value)) return kResultFalse; interval_=std::clamp(value,0,5);
    if (!stream.readInt32(value)) return kResultFalse; character_=std::clamp(value,0,2);
    if (!stream.readInt32(value)) return kResultFalse; muted_=value!=0;
    if (!stream.readInt32(value)) return kResultFalse; mixedNextPositive_=value!=0;
    if (!stream.readInt32(value)) return kResultFalse; phrasePositive_=value!=0;
    if (!stream.readInt32(value)) return kResultFalse; motivatorPos_=std::clamp(value,0,(int)kMotivatorCount);
    if (!stream.readInt32(value)) return kResultFalse; demotivatorPos_=std::clamp(value,0,(int)kDemotivatorCount);
    if (!stream.readInt32(value)) return kResultFalse; motivatorStart_=std::clamp(value,0,(int)kMotivatorCount-1);
    if (!stream.readInt32(value)) return kResultFalse; demotivatorStart_=std::clamp(value,0,(int)kDemotivatorCount-1);
    if (!stream.readInt32(value)) return kResultFalse; motivatorStep_=(value==53)?53:37;
    if (!stream.readInt32(value)) return kResultFalse; demotivatorStep_=(value==37)?37:53;
    if (!stream.readInt32(value)) return kResultFalse; currentPhraseGlobal_=std::clamp(value,0,(int)(kPhraseCount*2)-1);
    int32 soundValue=1;
    if (stream.readInt32(soundValue)) messageSound_=soundValue!=0;
    double volumeValue=0.5;
    if (stream.readDouble(volumeValue)) pingVolume_=std::clamp(volumeValue,0.0,1.0);
    nextState_=false; needsPhraseEmit_=true; resetIntervalCounter(); return kResultOk;
}

void MotivatoratorProcessor::handleParameters(ProcessData& data) {
    if (!data.inputParameterChanges) return;
    for (int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
        auto* queue=data.inputParameterChanges->getParameterData(i); if(!queue) continue;
        int32 offset=0; ParamValue value=0.0; if(queue->getPoint(queue->getPointCount()-1,offset,value)!=kResultTrue) continue;
        switch(queue->getParameterId()) {
            case kModeId:{int n=normalizedToIndex(value,3);if(n!=mode_){mode_=n;chooseNextPhrase();needsPhraseEmit_=true;resetIntervalCounter();}}break;
            case kLanguageId:{int n=normalizedToIndex(value,2);if(n!=language_){language_=n;chooseNextPhrase();needsPhraseEmit_=true;}}break;
            case kIntervalId:{int n=normalizedToIndex(value,6);if(n!=interval_){interval_=n;resetIntervalCounter();}}break;
            case kCharacterId: character_=normalizedToIndex(value,3); break;
            case kMuteId: muted_=value>=0.5; break;
            case kMessageSoundId: messageSound_=value>=0.5; if(!messageSound_) pingSamplesRemaining_=0; break;
            case kPingVolumeId: pingVolume_=std::clamp(value,0.0,1.0); break;
            case kNextId:{bool s=value>=0.5;if(s&&!nextState_){chooseNextPhrase();needsPhraseEmit_=true;resetIntervalCounter();}nextState_=s;}break;
            default: break;
        }
    }
}

int MotivatoratorProcessor::nextDeckIndex(bool motivator) {
    int& pos=motivator?motivatorPos_:demotivatorPos_; int& start=motivator?motivatorStart_:demotivatorStart_; int& step=motivator?motivatorStep_:demotivatorStep_;
    const int count=motivator?(int)kMotivatorCount:(int)kDemotivatorCount; const int index=(start+pos*step)%count;
    if(++pos>=count){pos=0;start=(start+11)%count;step=(step==37)?53:37;} return index;
}

void MotivatoratorProcessor::chooseNextPhrase(){
    if(mode_==kModeMotivator) phrasePositive_=true;
    else if(mode_==kModeDemotivator) phrasePositive_=false;
    else {
        mixedRandomState_ ^= mixedRandomState_ << 13;
        mixedRandomState_ ^= mixedRandomState_ >> 17;
        mixedRandomState_ ^= mixedRandomState_ << 5;
        phrasePositive_=(mixedRandomState_ & 1u)!=0u;
    }
    const int local=nextDeckIndex(phrasePositive_); const int languageBase=language_==0?0:(int)kPhraseCount;
    const int toneBase=phrasePositive_?0:(int)kMotivatorCount; currentPhraseGlobal_=languageBase+toneBase+local;
    triggerPing();
}

void MotivatoratorProcessor::emitPhrase(ProcessData& data){
    if(!data.outputParameterChanges){needsPhraseEmit_=false;return;} int32 idx=0;
    if(auto* q=data.outputParameterChanges->addParameterData(kPhraseId,idx)){int32 p=0;q->addPoint(0,(double)currentPhraseGlobal_/((kPhraseCount*2)-1),p);}
    if(auto* q=data.outputParameterChanges->addParameterData(kPhraseToneId,idx)){int32 p=0;q->addPoint(0,phrasePositive_?0.0:1.0,p);} needsPhraseEmit_=false;
}

void MotivatoratorProcessor::resetIntervalCounter(){
    static const int seconds[]={5,10,15,20,25,30};
    const int sec=seconds[std::clamp(interval_,0,5)];
    samplesUntilNext_=std::max<int64>(1,(int64)(sampleRate_*sec));
}

void MotivatoratorProcessor::triggerPing(){
    if(!messageSound_ || pingVolume_<=0.0) return;
    // Keep the direct ping short, but leave enough time for a tiny room tail.
    pingSamplesTotal_=std::max<int64>(1,(int64)(sampleRate_*0.20));
    pingSamplesRemaining_=pingSamplesTotal_;
    pingPhase_=0.0;
}

void MotivatoratorProcessor::mixPing(ProcessData& data){
    if(!messageSound_ || pingSamplesRemaining_<=0 || data.numOutputs<=0 || data.numSamples<=0) return;
    auto& out=data.outputs[0];
    const int64 startRemaining=pingSamplesRemaining_;
    const int32 count=std::min<int64>(data.numSamples,startRemaining);
    const double phaseInc=2.0*kPi*1650.0/sampleRate_;
    // Previous maximum was 0.22. 0.44 is +6.02 dB, giving the control useful headroom.
    const double baseGain=0.44*pingVolume_;
    for(int32 i=0;i<count;++i){
        const double elapsed=1.0-(double)pingSamplesRemaining_/(double)pingSamplesTotal_;
        const double elapsedSeconds=elapsed*0.20;

        // Original dry notification ping: essentially finished after ~90 ms.
        const double dryProgress=std::min(1.0,elapsedSeconds/0.09);
        const double attack=std::min(1.0,dryProgress/0.025);
        const double dryDecay=std::exp(-5.5*dryProgress);
        const double dry=(elapsedSeconds<=0.09)?std::sin(pingPhase_)*baseGain*attack*dryDecay:0.0;

        // Very small synthetic room tail. Three low-level, slightly detuned reflections
        // avoid a distinct echo while adding a short shimmer behind the ping.
        const double roomFade=std::exp(-18.0*elapsedSeconds);
        const double roomAttack=std::min(1.0,elapsedSeconds/0.012);
        const double reflections=(std::sin(pingPhase_*0.997+0.7)+std::sin(pingPhase_*1.013+1.9)+std::sin(pingPhase_*0.983+3.1))/3.0;
        const double room=reflections*baseGain*0.12*roomAttack*roomFade;
        const double sample=dry+room;

        if(data.symbolicSampleSize==kSample32){
            for(int32 c=0;c<out.numChannels;++c) if(out.channelBuffers32[c]) out.channelBuffers32[c][i]+=static_cast<float>(sample);
        } else if(data.symbolicSampleSize==kSample64){
            for(int32 c=0;c<out.numChannels;++c) if(out.channelBuffers64[c]) out.channelBuffers64[c][i]+=sample;
        }
        pingPhase_+=phaseInc; if(pingPhase_>=2.0*kPi) pingPhase_-=2.0*kPi;
        --pingSamplesRemaining_;
    }
}

tresult PLUGIN_API MotivatoratorProcessor::process(ProcessData& data){
    handleParameters(data);
    if(data.numInputs>0&&data.numOutputs>0){auto& in=data.inputs[0];auto& out=data.outputs[0];int ch=std::min(in.numChannels,out.numChannels);
        if(data.symbolicSampleSize==kSample32){for(int c=0;c<ch;++c)if(in.channelBuffers32[c]&&out.channelBuffers32[c])std::memcpy(out.channelBuffers32[c],in.channelBuffers32[c],sizeof(float)*data.numSamples);}
        else if(data.symbolicSampleSize==kSample64){for(int c=0;c<ch;++c)if(in.channelBuffers64[c]&&out.channelBuffers64[c])std::memcpy(out.channelBuffers64[c],in.channelBuffers64[c],sizeof(double)*data.numSamples);}}
    if(needsPhraseEmit_) emitPhrase(data);
    if(!muted_&&data.numSamples>0){samplesUntilNext_-=data.numSamples;if(samplesUntilNext_<=0){chooseNextPhrase();emitPhrase(data);resetIntervalCounter();}}
    mixPing(data);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorController::initialize(FUnknown* context){
    auto result=EditControllerEx1::initialize(context);if(result!=kResultOk)return result;
    auto* mode=new StringListParameter(STR16("Mode"),kModeId);mode->appendString(STR16("MOTIVATOR"));mode->appendString(STR16("DEMOTIVATOR"));mode->appendString(STR16("MIXED"));parameters.addParameter(mode);
    parameters.addParameter(STR16("Next"),nullptr,1,0.0,ParameterInfo::kCanAutomate,kNextId);parameters.addParameter(STR16("Mute Me"),nullptr,1,0.0,ParameterInfo::kCanAutomate,kMuteId);parameters.addParameter(STR16("Options"),nullptr,1,0.0,0,kOptionsId);
    auto* lang=new StringListParameter(STR16("Language"),kLanguageId);lang->appendString(STR16("Deutsch"));lang->appendString(STR16("English"));parameters.addParameter(lang);
    auto* interval=new StringListParameter(STR16("Interval"),kIntervalId);interval->appendString(STR16("5 sec"));interval->appendString(STR16("10 sec"));interval->appendString(STR16("15 sec"));interval->appendString(STR16("20 sec"));interval->appendString(STR16("25 sec"));interval->appendString(STR16("30 sec"));parameters.addParameter(interval);interval->setNormalized(0.4);
    auto* character=new StringListParameter(STR16("Character"),kCharacterId);character->appendString(STR16("GNOMI"));character->appendString(STR16("ROCKY"));character->appendString(STR16("D.O.M."));parameters.addParameter(character);
    parameters.addParameter(STR16("Phrase"),nullptr,(int32)(kPhraseCount*2-1),0.0,0,kPhraseId);
    auto* tone=new StringListParameter(STR16("Phrase Tone"),kPhraseToneId);tone->appendString(STR16("POSITIVE"));tone->appendString(STR16("NEGATIVE"));parameters.addParameter(tone);
    parameters.addParameter(STR16("Message Sound"),nullptr,1,1.0,ParameterInfo::kCanAutomate,kMessageSoundId);
    parameters.addParameter(STR16("Ping Volume"),STR16("%"),0,0.5,ParameterInfo::kCanAutomate,kPingVolumeId);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorController::setComponentState(IBStream* state){
    if(!state)return kInvalidArgument;IBStreamer s(state,kLittleEndian);int32 version=0,mode=0,language=0,interval=2,character=0,muted=0,mixed=1,positive=1,skip=0,current=0;
    if(!s.readInt32(version)||version!=kStateVersion)return kResultFalse;if(!s.readInt32(mode)||!s.readInt32(language)||!s.readInt32(interval)||!s.readInt32(character)||!s.readInt32(muted)||!s.readInt32(mixed)||!s.readInt32(positive))return kResultFalse;
    for(int i=0;i<6;++i)if(!s.readInt32(skip))return kResultFalse;if(!s.readInt32(current))return kResultFalse;
    setParamNormalized(kModeId,(double)std::clamp(mode,0,2)/2.0);setParamNormalized(kLanguageId,(double)std::clamp(language,0,1));setParamNormalized(kIntervalId,(double)std::clamp(interval,0,5)/5.0);setParamNormalized(kCharacterId,(double)std::clamp(character,0,2)/2.0);setParamNormalized(kMuteId,muted?1.0:0.0);setParamNormalized(kPhraseToneId,positive?0.0:1.0);setParamNormalized(kPhraseId,(double)std::clamp(current,0,(int)(kPhraseCount*2)-1)/((kPhraseCount*2)-1));
    int32 sound=1; if(s.readInt32(sound)) setParamNormalized(kMessageSoundId,sound?1.0:0.0);
    double volume=0.5; if(s.readDouble(volume)) setParamNormalized(kPingVolumeId,std::clamp(volume,0.0,1.0));
    return kResultOk;
}

IPlugView* PLUGIN_API MotivatoratorController::createView(FIDString name){if(name&&std::strcmp(name,ViewType::kEditor)==0)return new MotivatoratorEditor(this);return nullptr;}

} // namespace Steinberg::Vst
