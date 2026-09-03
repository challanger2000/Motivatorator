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
constexpr int kModeMotivator=0,kModeDemotivator=1,kModeMixed=2;
constexpr int32 kStateVersion=1;
constexpr double kPi=3.14159265358979323846;
int normalizedToIndex(ParamValue v,int count){return std::clamp(static_cast<int>(v*count),0,count-1);}
}

uint32_t MotivatoratorProcessor::nextRandom(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    if(state==0) state=0x9E3779B9u;
    return state;
}

MotivatoratorProcessor::MotivatoratorProcessor(){
    setControllerClass(MotivatoratorControllerUID);
    reshuffleDeck(true);
    reshuffleDeck(false);
}

tresult PLUGIN_API MotivatoratorProcessor::initialize(FUnknown* context){auto result=AudioEffect::initialize(context);if(result!=kResultOk)return result;addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);return kResultOk;}
tresult PLUGIN_API MotivatoratorProcessor::setupProcessing(ProcessSetup& setup){sampleRate_=setup.sampleRate>1.0?setup.sampleRate:44100.0;pingSamplesRemaining_=0;pingSamplesTotal_=0;pingPhase_=0.0;voicePrototype_.setSampleRate(sampleRate_);voicePrototype_.resetPlayback();return AudioEffect::setupProcessing(setup);}
tresult PLUGIN_API MotivatoratorProcessor::canProcessSampleSize(int32 s){return s==kSample32||s==kSample64?kResultTrue:kResultFalse;}

tresult PLUGIN_API MotivatoratorProcessor::getState(IBStream* state){
    if(!state)return kInvalidArgument;
    IBStreamer s(state,kLittleEndian);
    s.writeInt32(kStateVersion);
    s.writeInt32(mode_);
    s.writeInt32(language_);
    s.writeInt32(interval_);
    s.writeInt32(character_);
    s.writeInt32(muted_?1:0);
    s.writeInt32(1);
    s.writeInt32(phrasePositive_?1:0);
    // Keep the six legacy deck-state slots so older projects/controllers remain compatible.
    s.writeInt32(motivatorPos_);
    s.writeInt32(demotivatorPos_);
    s.writeInt32(0);
    s.writeInt32(0);
    s.writeInt32(0);
    s.writeInt32(0);
    s.writeInt32(currentPhraseGlobal_);
    s.writeInt32(messageSound_?1:0);
    s.writeDouble(pingVolume_);
    s.writeInt32(voiceEnabled_?1:0);
    s.writeDouble(voiceVolume_);
    return kResultOk;
}

tresult PLUGIN_API MotivatoratorProcessor::setState(IBStream* state){
    if(!state)return kInvalidArgument;
    IBStreamer s(state,kLittleEndian);
    int32 version=0,v=0;
    if(!s.readInt32(version)||version!=kStateVersion)return kResultFalse;
    if(!s.readInt32(v))return kResultFalse;mode_=std::clamp(v,0,2);
    if(!s.readInt32(v))return kResultFalse;language_=std::clamp(v,0,1);
    if(!s.readInt32(v))return kResultFalse;interval_=std::clamp(v,0,5);
    if(!s.readInt32(v))return kResultFalse;character_=std::clamp(v,0,2);
    if(!s.readInt32(v))return kResultFalse;muted_=v!=0;
    if(!s.readInt32(v))return kResultFalse;
    if(!s.readInt32(v))return kResultFalse;phrasePositive_=v!=0;
    // Read and intentionally discard the old deterministic deck state.
    for(int i=0;i<6;++i)if(!s.readInt32(v))return kResultFalse;
    if(!s.readInt32(v))return kResultFalse;currentPhraseGlobal_=std::clamp(v,0,(int)(kPhraseCount*2)-1);
    int32 sound=1;if(s.readInt32(sound))messageSound_=sound!=0;
    double pv=.5;if(s.readDouble(pv))pingVolume_=std::clamp(pv,0.0,1.0);
    int32 voice=1;if(s.readInt32(voice))voiceEnabled_=voice!=0;
    double vv=.5;if(s.readDouble(vv))voiceVolume_=std::clamp(vv,0.0,1.0);
    // A reload starts with fresh shuffled 500-card decks instead of restoring the old fixed-step sequence.
    reshuffleDeck(true);
    reshuffleDeck(false);
    needsPhraseEmit_=true;
    resetIntervalCounter();
    return kResultOk;
}

void MotivatoratorProcessor::handleParameters(ProcessData& data){if(!data.inputParameterChanges)return;for(int32 i=0;i<data.inputParameterChanges->getParameterCount();++i){auto* q=data.inputParameterChanges->getParameterData(i);if(!q||q->getPointCount()<=0)continue;int32 o=0;ParamValue value=0.;if(q->getPoint(q->getPointCount()-1,o,value)!=kResultTrue)continue;switch(q->getParameterId()){case kModeId:{int n=normalizedToIndex(value,3);if(n!=mode_){mode_=n;chooseNextPhrase();needsPhraseEmit_=true;resetIntervalCounter();}}break;case kLanguageId:{int n=normalizedToIndex(value,2);if(n!=language_){language_=n;chooseNextPhrase();needsPhraseEmit_=true;}}break;case kIntervalId:{int n=normalizedToIndex(value,6);if(n!=interval_){interval_=n;resetIntervalCounter();}}break;case kCharacterId:character_=normalizedToIndex(value,3);break;case kMuteId:muted_=value>=.5;break;case kMessageSoundId:messageSound_=value>=.5;if(!messageSound_)pingSamplesRemaining_=0;break;case kPingVolumeId:pingVolume_=std::clamp(value,0.,1.);break;case kVoiceEnabledId:voiceEnabled_=value>=.5;if(!voiceEnabled_)voicePrototype_.resetPlayback();break;case kVoiceVolumeId:voiceVolume_=std::clamp(value,0.,1.);break;default:break;}}}

void MotivatoratorProcessor::reshuffleDeck(bool motivator) noexcept {
    auto& deck=motivator?motivatorDeck_:demotivatorDeck_;
    auto& pos=motivator?motivatorPos_:demotivatorPos_;
    auto& rng=motivator?motivatorShuffleState_:demotivatorShuffleState_;
    const int count=motivator?static_cast<int>(kMotivatorCount):static_cast<int>(kDemotivatorCount);
    for(int i=0;i<count;++i)deck[static_cast<size_t>(i)]=static_cast<uint16_t>(i);
    for(int i=count-1;i>0;--i){
        const int j=static_cast<int>(nextRandom(rng)%static_cast<uint32_t>(i+1));
        std::swap(deck[static_cast<size_t>(i)],deck[static_cast<size_t>(j)]);
    }
    pos=0;
}

int MotivatoratorProcessor::nextDeckIndex(bool motivator) noexcept {
    auto& deck=motivator?motivatorDeck_:demotivatorDeck_;
    auto& pos=motivator?motivatorPos_:demotivatorPos_;
    const int count=motivator?static_cast<int>(kMotivatorCount):static_cast<int>(kDemotivatorCount);
    if(pos>=count)reshuffleDeck(motivator);
    return static_cast<int>(deck[static_cast<size_t>(pos++)]);
}

void MotivatoratorProcessor::chooseNextPhrase(){if(mode_==kModeMotivator)phrasePositive_=true;else if(mode_==kModeDemotivator)phrasePositive_=false;else{phrasePositive_=(nextRandom(mixedRandomState_)&1u)!=0u;}const int local=nextDeckIndex(phrasePositive_);const int languageBase=language_==0?0:(int)kPhraseCount;const int toneBase=phrasePositive_?0:(int)kMotivatorCount;currentPhraseGlobal_=languageBase+toneBase+local;triggerPing();if(voiceEnabled_)requestVoicePrototype();}
void MotivatoratorProcessor::requestVoicePrototype(){if(!voiceEnabled_)return;voicePrototype_.request(currentPhraseGlobal_,character_);}
void MotivatoratorProcessor::emitPhrase(ProcessData& data){if(!data.outputParameterChanges){needsPhraseEmit_=false;return;}int32 idx=0;if(auto* q=data.outputParameterChanges->addParameterData(kPhraseId,idx)){int32 p=0;q->addPoint(0,(double)currentPhraseGlobal_/((kPhraseCount*2)-1),p);}if(auto* q=data.outputParameterChanges->addParameterData(kPhraseToneId,idx)){int32 p=0;q->addPoint(0,phrasePositive_?0.:1.,p);}needsPhraseEmit_=false;}
void MotivatoratorProcessor::resetIntervalCounter(){static const int seconds[]={5,10,15,20,25,30};samplesUntilNext_=std::max<int64>(1,(int64)(sampleRate_*seconds[std::clamp(interval_,0,5)]));}
void MotivatoratorProcessor::triggerPing(){if(!messageSound_||pingVolume_<=0.)return;pingSamplesTotal_=std::max<int64>(1,(int64)(sampleRate_*.20));pingSamplesRemaining_=pingSamplesTotal_;pingPhase_=0.;}
void MotivatoratorProcessor::mixPing(ProcessData& data){if(!messageSound_||pingSamplesRemaining_<=0||data.numOutputs<=0||data.numSamples<=0)return;auto& out=data.outputs[0];const int32 count=std::min<int64>(data.numSamples,pingSamplesRemaining_);const double phaseInc=2.*kPi*1650./sampleRate_;const double baseGain=.44*pingVolume_;for(int32 i=0;i<count;++i){const double elapsed=1.-(double)pingSamplesRemaining_/(double)pingSamplesTotal_;const double es=elapsed*.20;const double dp=std::min(1.,es/.09);const double attack=std::min(1.,dp/.025);const double dryDecay=std::exp(-5.5*dp);const double dry=es<=.09?std::sin(pingPhase_)*baseGain*attack*dryDecay:0.;const double roomFade=std::exp(-18.*es);const double roomAttack=std::min(1.,es/.012);const double reflections=(std::sin(pingPhase_*.997+.7)+std::sin(pingPhase_*1.013+1.9)+std::sin(pingPhase_*.983+3.1))/3.;const double sample=dry+reflections*baseGain*.12*roomAttack*roomFade;if(data.symbolicSampleSize==kSample32){for(int32 c=0;c<out.numChannels;++c)if(out.channelBuffers32[c])out.channelBuffers32[c][i]+=static_cast<float>(sample);}else if(data.symbolicSampleSize==kSample64){for(int32 c=0;c<out.numChannels;++c)if(out.channelBuffers64[c])out.channelBuffers64[c][i]+=sample;}pingPhase_+=phaseInc;if(pingPhase_>=2.*kPi)pingPhase_-=2.*kPi;--pingSamplesRemaining_;}}
void MotivatoratorProcessor::mixVoicePrototype(ProcessData& data){if(!voiceEnabled_||voiceVolume_<=0.||data.numOutputs<=0||data.numSamples<=0)return;auto& out=data.outputs[0];const double gain=.70*voiceVolume_;for(int32 i=0;i<data.numSamples;++i){const double sample=(double)voicePrototype_.nextSample()*gain;if(sample==0.)continue;if(data.symbolicSampleSize==kSample32){for(int32 c=0;c<out.numChannels;++c)if(out.channelBuffers32[c])out.channelBuffers32[c][i]+=static_cast<float>(sample);}else if(data.symbolicSampleSize==kSample64){for(int32 c=0;c<out.numChannels;++c)if(out.channelBuffers64[c])out.channelBuffers64[c][i]+=sample;}}}

tresult PLUGIN_API MotivatoratorProcessor::process(ProcessData& data){handleParameters(data);if(data.numInputs>0&&data.numOutputs>0){auto& in=data.inputs[0];auto& out=data.outputs[0];int ch=std::min(in.numChannels,out.numChannels);if(data.symbolicSampleSize==kSample32){for(int c=0;c<ch;++c)if(in.channelBuffers32[c]&&out.channelBuffers32[c])std::memcpy(out.channelBuffers32[c],in.channelBuffers32[c],sizeof(float)*data.numSamples);}else if(data.symbolicSampleSize==kSample64){for(int c=0;c<ch;++c)if(in.channelBuffers64[c]&&out.channelBuffers64[c])std::memcpy(out.channelBuffers64[c],in.channelBuffers64[c],sizeof(double)*data.numSamples);}}if(needsPhraseEmit_)emitPhrase(data);if(!muted_&&data.numSamples>0){samplesUntilNext_-=data.numSamples;if(samplesUntilNext_<=0){chooseNextPhrase();emitPhrase(data);resetIntervalCounter();}}mixPing(data);mixVoicePrototype(data);return kResultOk;}

tresult PLUGIN_API MotivatoratorController::initialize(FUnknown* context){auto result=EditControllerEx1::initialize(context);if(result!=kResultOk)return result;auto* mode=new StringListParameter(STR16("Mode"),kModeId);mode->appendString(STR16("MOTIVATOR"));mode->appendString(STR16("DEMOTIVATOR"));mode->appendString(STR16("MIXED"));parameters.addParameter(mode);parameters.addParameter(STR16("Mute Me"),nullptr,1,0.,ParameterInfo::kCanAutomate,kMuteId);parameters.addParameter(STR16("Options"),nullptr,1,0.,0,kOptionsId);auto* lang=new StringListParameter(STR16("Language"),kLanguageId);lang->appendString(STR16("Deutsch"));lang->appendString(STR16("English"));parameters.addParameter(lang);auto* interval=new StringListParameter(STR16("Interval"),kIntervalId);interval->appendString(STR16("5 sec"));interval->appendString(STR16("10 sec"));interval->appendString(STR16("15 sec"));interval->appendString(STR16("20 sec"));interval->appendString(STR16("25 sec"));interval->appendString(STR16("30 sec"));parameters.addParameter(interval);interval->setNormalized(.4);auto* character=new StringListParameter(STR16("Character"),kCharacterId);character->appendString(STR16("GNOMI"));character->appendString(STR16("ROCKY"));character->appendString(STR16("D.O.M."));parameters.addParameter(character);parameters.addParameter(STR16("Phrase"),nullptr,(int32)(kPhraseCount*2-1),0.,0,kPhraseId);auto* tone=new StringListParameter(STR16("Phrase Tone"),kPhraseToneId);tone->appendString(STR16("POSITIVE"));tone->appendString(STR16("NEGATIVE"));parameters.addParameter(tone);parameters.addParameter(STR16("Ping"),nullptr,1,1.,ParameterInfo::kCanAutomate,kMessageSoundId);parameters.addParameter(STR16("Ping Volume"),STR16("%"),0,.5,ParameterInfo::kCanAutomate,kPingVolumeId);parameters.addParameter(STR16("Voice"),nullptr,1,1.,ParameterInfo::kCanAutomate,kVoiceEnabledId);parameters.addParameter(STR16("Voice Volume"),STR16("%"),0,.5,ParameterInfo::kCanAutomate,kVoiceVolumeId);return kResultOk;}

tresult PLUGIN_API MotivatoratorController::setComponentState(IBStream* state){if(!state)return kInvalidArgument;IBStreamer s(state,kLittleEndian);int32 version=0,mode=0,language=0,interval=2,character=0,muted=0,legacyMixed=1,positive=1,skip=0,current=0;if(!s.readInt32(version)||version!=kStateVersion)return kResultFalse;if(!s.readInt32(mode)||!s.readInt32(language)||!s.readInt32(interval)||!s.readInt32(character)||!s.readInt32(muted)||!s.readInt32(legacyMixed)||!s.readInt32(positive))return kResultFalse;for(int i=0;i<6;++i)if(!s.readInt32(skip))return kResultFalse;if(!s.readInt32(current))return kResultFalse;setParamNormalized(kModeId,(double)std::clamp(mode,0,2)/2.);setParamNormalized(kLanguageId,(double)std::clamp(language,0,1));setParamNormalized(kIntervalId,(double)std::clamp(interval,0,5)/5.);setParamNormalized(kCharacterId,(double)std::clamp(character,0,2)/2.);setParamNormalized(kMuteId,muted?1.:0.);setParamNormalized(kPhraseToneId,positive?0.:1.);setParamNormalized(kPhraseId,(double)std::clamp(current,0,(int)(kPhraseCount*2)-1)/((kPhraseCount*2)-1));int32 sound=1;if(s.readInt32(sound))setParamNormalized(kMessageSoundId,sound?1.:0.);double pv=.5;if(s.readDouble(pv))setParamNormalized(kPingVolumeId,std::clamp(pv,0.,1.));int32 voice=1;if(s.readInt32(voice))setParamNormalized(kVoiceEnabledId,voice?1.:0.);else setParamNormalized(kVoiceEnabledId,1.);double vv=.5;if(s.readDouble(vv))setParamNormalized(kVoiceVolumeId,std::clamp(vv,0.,1.));else setParamNormalized(kVoiceVolumeId,.5);return kResultOk;}
IPlugView* PLUGIN_API MotivatoratorController::createView(FIDString name){if(name&&std::strcmp(name,ViewType::kEditor)==0)return new MotivatoratorEditor(this);return nullptr;}
} // namespace Steinberg::Vst
