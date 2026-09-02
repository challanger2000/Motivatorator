#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "PhraseBank.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/uidescription/uiattributes.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace Steinberg::Vst {
namespace {

static void setParameter(EditController* controller, ParamID paramId, double value) {
    if (!controller) return;
    controller->beginEdit(paramId);
    controller->setParamNormalized(paramId, value);
    controller->performEdit(paramId, value);
    controller->endEdit(paramId);
}

class CharacterView final : public VSTGUI::CView {
public:
    CharacterView(const VSTGUI::CRect& size, EditController* controller) : CView(size), controller_(controller) {
        gnomiPositive_=load("gnomi_positive.png"); gnomiNegative_=load("gnomi_negative.png");
        rockyPositive_=load("rocky_positive.png"); rockyNegative_=load("rocky_negative.png");
        domPositive_=load("dom_positive.png"); domNegative_=load("dom_negative.png");
        setMouseEnabled(false);
        timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);
    }
    void draw(VSTGUI::CDrawContext* context) override {
        if(!controller_){setDirty(false);return;}
        const int mode=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kModeId)*2.0)),0,2);
        const int character=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kCharacterId)*2.0)),0,2);
        const bool negative=mode==1||(mode==2&&controller_->getParamNormalized(kPhraseToneId)>=0.5);
        VSTGUI::SharedPointer<VSTGUI::CBitmap> bitmap;
        if(character==0) bitmap=negative?gnomiNegative_:gnomiPositive_;
        else if(character==1) bitmap=negative?rockyNegative_:rockyPositive_;
        else bitmap=negative?domNegative_:domPositive_;
        if(bitmap) bitmap->draw(context,getViewSize(),VSTGUI::CPoint(0.,0.),1.f);
        setDirty(false);
    }
private:
    static VSTGUI::SharedPointer<VSTGUI::CBitmap> load(const char* name){
        auto bitmap=VSTGUI::makeOwned<VSTGUI::CBitmap>(VSTGUI::CResourceDescription(name));
        if(bitmap&&bitmap->getPlatformBitmap()) bitmap->getPlatformBitmap()->setScaleFactor(2.0);
        return bitmap;
    }
    EditController* controller_{nullptr};
    VSTGUI::SharedPointer<VSTGUI::CBitmap> gnomiPositive_,gnomiNegative_,rockyPositive_,rockyNegative_,domPositive_,domNegative_;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class ModeTitleView final : public VSTGUI::CView {
public:
    ModeTitleView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){
        setMouseEnabled(false);
        timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);
    }
    void draw(VSTGUI::CDrawContext* context) override {
        if(!controller_){setDirty(false);return;}
        const int mode=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kModeId)*2.0)),0,2);
        const char* text=mode==0?"MOTIVATOR":(mode==1?"DEMOTIVATOR":"MIXED");
        VSTGUI::CColor color=mode==0?VSTGUI::CColor(255,184,70,255):(mode==1?VSTGUI::CColor(238,72,45,255):VSTGUI::CColor(255,137,48,255));
        const auto rect=getViewSize();
        context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",22.,VSTGUI::kBoldFace));
        context->setDrawMode(VSTGUI::kAntiAliasing);
        VSTGUI::CColor glow=color; glow.alpha=52; context->setFontColor(glow);
        for(double dx=-2.;dx<=2.;dx+=2.) for(double dy=-2.;dy<=2.;dy+=2.) { if(dx==0.&&dy==0.) continue; auto r=rect; r.offset(dx,dy); context->drawString(text,r,VSTGUI::kCenterText,true); }
        glow.alpha=105; context->setFontColor(glow);
        for(double dx=-1.;dx<=1.;dx+=2.) for(double dy=-1.;dy<=1.;dy+=2.) { auto r=rect; r.offset(dx,dy); context->drawString(text,r,VSTGUI::kCenterText,true); }
        context->setFontColor(color); context->drawString(text,rect,VSTGUI::kCenterText,true); setDirty(false);
    }
private:
    EditController* controller_{nullptr};
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class PhraseView final : public VSTGUI::CView {
public:
    PhraseView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){
        setMouseEnabled(false);
        timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){if(fadeTicks_<4)++fadeTicks_;invalid();},100);
    }
    void draw(VSTGUI::CDrawContext* context) override {
        if(!controller_){setDirty(false);return;}
        const int total=static_cast<int>(MotivatoratorPhrases::kPhraseCount*2);
        const int global=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kPhraseId)*(total-1))),0,total-1);
        if(global!=lastPhrase_){lastPhrase_=global;fadeTicks_=0;}
        const double fade=std::clamp(fadeTicks_/3.5,0.0,1.0);
        const int perLanguage=static_cast<int>(MotivatoratorPhrases::kPhraseCount);
        const bool english=global>=perLanguage; const int local=global%perLanguage;
        const bool positive=local<static_cast<int>(MotivatoratorPhrases::kMotivatorCount);
        const int phraseIndex=positive?local:local-static_cast<int>(MotivatoratorPhrases::kMotivatorCount);
        const auto& phrase=positive?MotivatoratorPhrases::kMotivator[phraseIndex]:MotivatoratorPhrases::kDemotivator[phraseIndex];
        const Steinberg::Vst::TChar* text=english?phrase.en:phrase.de;
        VSTGUI::CColor color=positive?VSTGUI::CColor(255,174,62,255):VSTGUI::CColor(238,76,48,255); color.alpha=static_cast<uint8_t>(255.0*fade);
        context->setDrawMode(VSTGUI::kAntiAliasing); auto safe=getViewSize(); safe.left+=18.; safe.right-=18.; safe.top+=12.; safe.bottom-=12.;
        double fontSize=30.; std::vector<std::u16string> lines;
        for(;fontSize>=16.;fontSize-=2.) { context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Courier New",fontSize,VSTGUI::kBoldFace)); lines=wrapToWidth(context,text,safe.getWidth()); if(fontSize*1.24*lines.size()<=safe.getHeight()) break; }
        fontSize=std::max(16.,fontSize); context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Courier New",fontSize,VSTGUI::kBoldFace)); lines=wrapToWidth(context,text,safe.getWidth());
        const double lineH=fontSize*1.24; const double totalH=lineH*lines.size(); double y=safe.top+(safe.getHeight()-totalH)*0.5;
        for(const auto& line:lines) { VSTGUI::CRect r(safe.left,y,safe.right,y+lineH); const VSTGUI::UTF8String utf8(toUtf8(line)); VSTGUI::CColor glow=color; glow.alpha=static_cast<uint8_t>(46.0*fade); context->setFontColor(glow);
            for(double dx=-2.;dx<=2.;dx+=2.) for(double dy=-2.;dy<=2.;dy+=2.) { if(dx==0.&&dy==0.) continue; auto h=r; h.offset(dx,dy); context->drawString(utf8,h,VSTGUI::kCenterText,true); }
            context->setFontColor(color); context->drawString(utf8,r,VSTGUI::kCenterText,true); y+=lineH; }
        setDirty(false);
    }
private:
    static std::vector<std::u16string> wrapToWidth(VSTGUI::CDrawContext* context,const Steinberg::Vst::TChar* text,double maxWidth){
        std::u16string src(reinterpret_cast<const char16_t*>(text)); std::vector<std::u16string> out; std::u16string line; size_t pos=0;
        while(pos<src.size()){ while(pos<src.size()&&src[pos]==u' ')++pos; if(pos>=src.size())break; size_t end=src.find(u' ',pos); if(end==std::u16string::npos)end=src.size(); std::u16string word=src.substr(pos,end-pos); std::u16string candidate=line.empty()?word:line+u" "+word; if(!line.empty()&&context->getStringWidth(VSTGUI::UTF8String(toUtf8(candidate)))>maxWidth){out.push_back(line);line=word;} else line=candidate; pos=end; }
        if(!line.empty())out.push_back(line); if(out.empty())out.push_back(u""); return out;
    }
    static std::string toUtf8(const std::u16string& s){ std::string out; for(char16_t c:s){ if(c<0x80)out.push_back(static_cast<char>(c)); else if(c<0x800){out.push_back(static_cast<char>(0xC0|(c>>6)));out.push_back(static_cast<char>(0x80|(c&0x3F)));} else{out.push_back(static_cast<char>(0xE0|(c>>12)));out.push_back(static_cast<char>(0x80|((c>>6)&0x3F)));out.push_back(static_cast<char>(0x80|(c&0x3F)));} } return out; }
    EditController* controller_{nullptr}; int lastPhrase_{-1}; int fadeTicks_{4}; VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

static void drawButtonGlow(VSTGUI::CDrawContext* context,VSTGUI::CRect rect,const VSTGUI::CColor& color){ rect.inset(2.,2.); VSTGUI::CColor fill=color; fill.alpha=58; context->setFillColor(fill); context->drawRect(rect,VSTGUI::kDrawFilled); VSTGUI::CColor edge=color; edge.alpha=245; context->setFrameColor(edge); context->setLineWidth(1.0); context->drawRect(rect,VSTGUI::kDrawStroked); auto h1=rect; h1.inset(1.,1.); VSTGUI::CColor c1=color; c1.alpha=150; context->setFrameColor(c1); context->drawRect(h1,VSTGUI::kDrawStroked); auto h2=rect; h2.inset(3.,3.); VSTGUI::CColor c2=color; c2.alpha=88; context->setFrameColor(c2); context->drawRect(h2,VSTGUI::kDrawStroked); auto h3=rect; h3.inset(5.,5.); VSTGUI::CColor c3=color; c3.alpha=42; context->setFrameColor(c3); context->drawRect(h3,VSTGUI::kDrawStroked); }

class ParameterButtonView final:public VSTGUI::CView {
public:
    ParameterButtonView(const VSTGUI::CRect& size,EditController* controller,ParamID paramId,double value,const VSTGUI::CColor& glowColor):CView(size),controller_(controller),paramId_(paramId),value_(value),glowColor_(glowColor){setMouseEnabled(true);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context)override{if(controller_&&std::abs(controller_->getParamNormalized(paramId_)-value_)<0.20)drawButtonGlow(context,getViewSize(),glowColor_);setDirty(false);}
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&)override{if(!controller_)return VSTGUI::kMouseEventNotHandled;setParameter(controller_,paramId_,value_);invalid();return VSTGUI::kMouseEventHandled;}
private: EditController* controller_{nullptr}; ParamID paramId_{0}; double value_{0.0}; VSTGUI::CColor glowColor_; VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class OptionsButtonView final : public VSTGUI::CView {
public:
    OptionsButtonView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){setMouseEnabled(true);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context)override{if(controller_&&controller_->getParamNormalized(kOptionsId)>=0.5)drawButtonGlow(context,getViewSize(),VSTGUI::CColor(255,156,38,255));setDirty(false);}
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&)override{if(!controller_)return VSTGUI::kMouseEventNotHandled;const double next=controller_->getParamNormalized(kOptionsId)>=0.5?0.0:1.0;setParameter(controller_,kOptionsId,next);invalid();return VSTGUI::kMouseEventHandled;}
private: EditController* controller_{nullptr}; VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class OptionsPanelView final : public VSTGUI::CView {
public:
    OptionsPanelView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){setMouseEnabled(true);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context)override{
        if(!isOpen()){setDirty(false);return;} context->setDrawMode(VSTGUI::kAntiAliasing); auto panel=getViewSize(); panel.inset(18.,12.);
        VSTGUI::CColor bg(8,8,7,242);context->setFillColor(bg);context->drawRect(panel,VSTGUI::kDrawFilled);VSTGUI::CColor frame(178,108,38,225);context->setFrameColor(frame);context->setLineWidth(1.0);context->drawRect(panel,VSTGUI::kDrawStroked);auto inner=panel;inner.inset(3.,3.);frame.alpha=75;context->setFrameColor(frame);context->drawRect(inner,VSTGUI::kDrawStroked);
        const VSTGUI::CColor text(255,186,78,255);context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",18.,VSTGUI::kBoldFace));context->setFontColor(text);VSTGUI::CRect title(panel.left,panel.top+5.,panel.right,panel.top+27.);context->drawString("OPTIONS",title,VSTGUI::kCenterText,true);
        context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",12.,VSTGUI::kBoldFace));
        context->drawString("LANGUAGE",VSTGUI::CRect(panel.left+14.,panel.top+34.,panel.left+92.,panel.top+54.),VSTGUI::kLeftText,true);drawChoice(context,languageRect(0),"DEUTSCH",languageIndex()==0);drawChoice(context,languageRect(1),"ENGLISH",languageIndex()==1);
        context->drawString("INTERVAL",VSTGUI::CRect(panel.left+14.,panel.top+76.,panel.left+92.,panel.top+96.),VSTGUI::kLeftText,true);static const char* labels[6]={"5s","10s","15s","20s","25s","30s"};const int selected=intervalIndex();for(int i=0;i<6;++i)drawChoice(context,intervalRect(i),labels[i],selected==i);
        context->drawString("MESSAGE SOUND",VSTGUI::CRect(panel.left+14.,panel.top+123.,panel.left+125.,panel.top+143.),VSTGUI::kLeftText,true);drawChoice(context,soundRect(0),"OFF",!soundOn());drawChoice(context,soundRect(1),"ON",soundOn());
        context->drawString("PING VOLUME",VSTGUI::CRect(panel.left+14.,panel.top+166.,panel.left+112.,panel.top+186.),VSTGUI::kLeftText,true);drawVolume(context);
        setDirty(false);
    }
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where,const VSTGUI::CButtonState&)override{
        if(!isOpen()||!controller_)return VSTGUI::kMouseEventNotHandled;for(int i=0;i<2;++i)if(languageRect(i).pointInside(where)){setParameter(controller_,kLanguageId,(double)i);invalid();return VSTGUI::kMouseEventHandled;}for(int i=0;i<6;++i)if(intervalRect(i).pointInside(where)){setParameter(controller_,kIntervalId,(double)i/5.0);invalid();return VSTGUI::kMouseEventHandled;}if(soundRect(0).pointInside(where)){setParameter(controller_,kMessageSoundId,0.0);invalid();return VSTGUI::kMouseEventHandled;}if(soundRect(1).pointInside(where)){setParameter(controller_,kMessageSoundId,1.0);invalid();return VSTGUI::kMouseEventHandled;}if(volumeRect().pointInside(where)){setVolumeFromPoint(where);return VSTGUI::kMouseEventHandled;}return VSTGUI::kMouseEventHandled;
    }
    VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where,const VSTGUI::CButtonState& buttons)override{if(isOpen()&&buttons.isLeftButton()&&volumeRect().pointInside(where)){setVolumeFromPoint(where);return VSTGUI::kMouseEventHandled;}return VSTGUI::kMouseEventNotHandled;}
private:
    bool isOpen()const{return controller_&&controller_->getParamNormalized(kOptionsId)>=0.5;}int languageIndex()const{return std::clamp((int)std::lround(controller_->getParamNormalized(kLanguageId)),0,1);}int intervalIndex()const{return std::clamp((int)std::lround(controller_->getParamNormalized(kIntervalId)*5.0),0,5);}bool soundOn()const{return controller_->getParamNormalized(kMessageSoundId)>=0.5;}
    VSTGUI::CRect languageRect(int i)const{auto p=getViewSize();p.inset(18.,12.);const double w=82.,gap=8.,left=p.left+92.+i*(w+gap);return VSTGUI::CRect(left,p.top+30.,left+w,p.top+56.);}VSTGUI::CRect intervalRect(int i)const{auto p=getViewSize();p.inset(18.,12.);const double gap=4.,total=p.getWidth()-28.,w=(total-gap*5.)/6.,left=p.left+14.+i*(w+gap);return VSTGUI::CRect(left,p.top+96.,left+w,p.top+122.);}VSTGUI::CRect soundRect(int i)const{auto p=getViewSize();p.inset(18.,12.);const double w=64.,gap=7.,left=p.left+126.+i*(w+gap);return VSTGUI::CRect(left,p.top+119.,left+w,p.top+145.);}VSTGUI::CRect volumeRect()const{auto p=getViewSize();p.inset(18.,12.);return VSTGUI::CRect(p.left+112.,p.top+164.,p.right-14.,p.top+187.);}
    void setVolumeFromPoint(const VSTGUI::CPoint& where){auto r=volumeRect();double v=std::clamp((where.x-r.left)/r.getWidth(),0.0,1.0);setParameter(controller_,kPingVolumeId,v);invalid();}
    void drawChoice(VSTGUI::CDrawContext* c,VSTGUI::CRect r,const char* label,bool selected)const{VSTGUI::CColor fill=selected?VSTGUI::CColor(122,63,18,185):VSTGUI::CColor(24,22,18,220);c->setFillColor(fill);c->drawRect(r,VSTGUI::kDrawFilled);VSTGUI::CColor edge=selected?VSTGUI::CColor(255,151,44,245):VSTGUI::CColor(116,83,50,190);c->setFrameColor(edge);c->setLineWidth(1.);c->drawRect(r,VSTGUI::kDrawStroked);c->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",11.,VSTGUI::kBoldFace));c->setFontColor(selected?VSTGUI::CColor(255,194,93,255):VSTGUI::CColor(199,167,126,255));c->drawString(label,r,VSTGUI::kCenterText,true);}
    void drawVolume(VSTGUI::CDrawContext* c)const{auto r=volumeRect();VSTGUI::CColor bg(24,22,18,230);c->setFillColor(bg);c->drawRect(r,VSTGUI::kDrawFilled);VSTGUI::CColor edge(116,83,50,210);c->setFrameColor(edge);c->drawRect(r,VSTGUI::kDrawStroked);double v=std::clamp(controller_->getParamNormalized(kPingVolumeId),0.0,1.0);auto fill=r;fill.right=fill.left+r.getWidth()*v;fill.inset(2.,2.);VSTGUI::CColor amber(180,87,20,210);c->setFillColor(amber);c->drawRect(fill,VSTGUI::kDrawFilled);char value[16];std::snprintf(value,sizeof(value),"%d%%",(int)std::lround(v*100.));c->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",11.,VSTGUI::kBoldFace));c->setFontColor(VSTGUI::CColor(255,205,120,255));c->drawString(value,r,VSTGUI::kCenterText,true);}
    EditController* controller_{nullptr};VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

} // namespace

MotivatoratorEditor::MotivatoratorEditor(EditController* controller):VST3Editor(controller,"view","Motivatorator.uidesc"),controller_(controller){}
VSTGUI::CView* MotivatoratorEditor::createView(const VSTGUI::UIAttributes& attributes,const VSTGUI::IUIDescription* description){if(const auto name=attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName)){if(*name=="CharacterView")return new CharacterView(VSTGUI::CRect(18.,82.,335.,352.),controller_);if(*name=="ModeTitleView")return new ModeTitleView(VSTGUI::CRect(100.,10.,304.,48.),controller_);if(*name=="PhraseView")return new PhraseView(VSTGUI::CRect(315.,90.,665.,330.),controller_);if(*name=="ModeMotivator")return new ParameterButtonView(VSTGUI::CRect(242.,368.,312.,409.),controller_,kModeId,0.0,VSTGUI::CColor(255,177,45,255));if(*name=="ModeDemotivator")return new ParameterButtonView(VSTGUI::CRect(315.,369.,399.,408.),controller_,kModeId,0.5,VSTGUI::CColor(230,56,36,255));if(*name=="ModeMixed")return new ParameterButtonView(VSTGUI::CRect(397.,367.,470.,410.),controller_,kModeId,1.0,VSTGUI::CColor(255,125,35,255));if(*name=="CharacterGnomi")return new ParameterButtonView(VSTGUI::CRect(500.,369.,574.,408.),controller_,kCharacterId,0.0,VSTGUI::CColor(255,156,38,255));if(*name=="CharacterRocky")return new ParameterButtonView(VSTGUI::CRect(579.,369.,653.,408.),controller_,kCharacterId,0.5,VSTGUI::CColor(255,156,38,255));if(*name=="CharacterDom")return new ParameterButtonView(VSTGUI::CRect(658.,369.,735.,408.),controller_,kCharacterId,1.0,VSTGUI::CColor(255,156,38,255));if(*name=="OptionsButton")return new OptionsButtonView(VSTGUI::CRect(580.,8.,710.,52.),controller_);if(*name=="OptionsPanel")return new OptionsPanelView(VSTGUI::CRect(315.,90.,665.,330.),controller_);}return VSTGUI::VST3Editor::createView(attributes,description);}

} // namespace Steinberg::Vst
