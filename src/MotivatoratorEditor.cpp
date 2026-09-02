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
#include <string>
#include <vector>

namespace Steinberg::Vst {
namespace {
class CharacterView final : public VSTGUI::CView {
public:
    CharacterView(const VSTGUI::CRect& size, EditController* controller) : CView(size), controller_(controller) {gnomiPositive_=load("gnomi_positive.png");gnomiNegative_=load("gnomi_negative.png");rockyPositive_=load("rocky_positive.png");rockyNegative_=load("rocky_negative.png");domPositive_=load("dom_positive.png");domNegative_=load("dom_negative.png");setMouseEnabled(false);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context) override {if(!controller_){setDirty(false);return;}const int mode=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kModeId)*2.0)),0,2);const int character=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kCharacterId)*2.0)),0,2);const bool negative=mode==1||(mode==2&&controller_->getParamNormalized(kPhraseToneId)>=0.5);VSTGUI::SharedPointer<VSTGUI::CBitmap> bitmap;if(character==0)bitmap=negative?gnomiNegative_:gnomiPositive_;else if(character==1)bitmap=negative?rockyNegative_:rockyPositive_;else bitmap=negative?domNegative_:domPositive_;if(bitmap)bitmap->draw(context,getViewSize(),VSTGUI::CPoint(0.,0.),1.f);setDirty(false);}
private:static VSTGUI::SharedPointer<VSTGUI::CBitmap> load(const char* name){auto bitmap=VSTGUI::makeOwned<VSTGUI::CBitmap>(VSTGUI::CResourceDescription(name));if(bitmap&&bitmap->getPlatformBitmap())bitmap->getPlatformBitmap()->setScaleFactor(2.0);return bitmap;}EditController* controller_{nullptr};VSTGUI::SharedPointer<VSTGUI::CBitmap> gnomiPositive_,gnomiNegative_,rockyPositive_,rockyNegative_,domPositive_,domNegative_;VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};
class ModeTitleView final : public VSTGUI::CView {
public:ModeTitleView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){setMouseEnabled(false);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}void draw(VSTGUI::CDrawContext* context) override {if(!controller_){setDirty(false);return;}const int mode=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kModeId)*2.0)),0,2);const char* text=mode==0?"MOTIVATOR":(mode==1?"DEMOTIVATOR":"MIXED");VSTGUI::CColor color=mode==0?VSTGUI::CColor(255,184,70,255):(mode==1?VSTGUI::CColor(238,72,45,255):VSTGUI::CColor(255,137,48,255));const auto rect=getViewSize();context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Arial",22.,VSTGUI::kBoldFace));context->setDrawMode(VSTGUI::kAntiAliasing);VSTGUI::CColor glow=color;glow.alpha=52;context->setFontColor(glow);for(double dx=-2.;dx<=2.;dx+=2.)for(double dy=-2.;dy<=2.;dy+=2.){if(dx==0.&&dy==0.)continue;auto r=rect;r.offset(dx,dy);context->drawString(text,r,VSTGUI::kCenterText,true);}glow.alpha=105;context->setFontColor(glow);for(double dx=-1.;dx<=1.;dx+=2.)for(double dy=-1.;dy<=1.;dy+=2.){auto r=rect;r.offset(dx,dy);context->drawString(text,r,VSTGUI::kCenterText,true);}context->setFontColor(color);context->drawString(text,rect,VSTGUI::kCenterText,true);setDirty(false);}private:EditController* controller_{nullptr};VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};
class PhraseView final : public VSTGUI::CView {
public:
    PhraseView(const VSTGUI::CRect& size,EditController* controller):CView(size),controller_(controller){setMouseEnabled(false);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context) override {
        if(!controller_){setDirty(false);return;}
        const int total=static_cast<int>(MotivatoratorPhrases::kPhraseCount*2);
        const int global=std::clamp(static_cast<int>(std::lround(controller_->getParamNormalized(kPhraseId)*(total-1))),0,total-1);
        const int perLanguage=static_cast<int>(MotivatoratorPhrases::kPhraseCount);
        const bool english=global>=perLanguage;
        const int local=global%perLanguage;
        const bool positive=local<static_cast<int>(MotivatoratorPhrases::kMotivatorCount);
        const int phraseIndex=positive?local:local-static_cast<int>(MotivatoratorPhrases::kMotivatorCount);
        const auto& phrase=positive?MotivatoratorPhrases::kMotivator[phraseIndex]:MotivatoratorPhrases::kDemotivator[phraseIndex];
        const Steinberg::Vst::TChar* text=english?phrase.en:phrase.de;

        const double fontSize=chooseFontSize(text);
        context->setFont(VSTGUI::makeOwned<VSTGUI::CFontDesc>("Courier New",fontSize,VSTGUI::kBoldFace));
        context->setDrawMode(VSTGUI::kAntiAliasing);
        VSTGUI::CColor color=positive?VSTGUI::CColor(255,174,62,255):VSTGUI::CColor(238,76,48,255);

        // Fixed safe area inside the visible CRT glass. Text is never allowed to use the bezel edges.
        auto safe=getViewSize();
        safe.left+=18.; safe.right-=18.; safe.top+=12.; safe.bottom-=12.;
        auto lines=wrap(text,maxCharsFor(fontSize));
        const double lineH=fontSize*1.24;
        const double totalH=lineH*lines.size();
        double y=safe.top+(safe.getHeight()-totalH)*0.5;

        for(const auto& line:lines){
            VSTGUI::CRect r(safe.left,y,safe.right,y+lineH);
            const VSTGUI::UTF8String utf8(toUtf8(line));
            VSTGUI::CColor glow=color; glow.alpha=46; context->setFontColor(glow);
            for(double dx=-2.;dx<=2.;dx+=2.) for(double dy=-2.;dy<=2.;dy+=2.) {if(dx==0.&&dy==0.)continue;auto h=r;h.offset(dx,dy);context->drawString(utf8,h,VSTGUI::kCenterText,true);}
            context->setFontColor(color); context->drawString(utf8,r,VSTGUI::kCenterText,true); y+=lineH;
        }
        setDirty(false);
    }
private:
    static double chooseFontSize(const Steinberg::Vst::TChar* text){size_t n=0;while(text[n])++n;if(n<=30)return 23.;if(n<=46)return 20.;if(n<=64)return 18.;return 16.;}
    // Conservative limits for Courier New inside the 314px-wide CRT safe area.
    static size_t maxCharsFor(double s){return s>=23.?21:(s>=20.?24:(s>=18.?27:31));}
    static std::vector<std::u16string> wrap(const Steinberg::Vst::TChar* text,size_t maxChars){std::u16string src(reinterpret_cast<const char16_t*>(text));std::vector<std::u16string> out;size_t pos=0;while(pos<src.size()){while(pos<src.size()&&src[pos]==u' ')++pos;if(pos>=src.size())break;size_t end=std::min(pos+maxChars,src.size());if(end<src.size()){size_t space=src.rfind(u' ',end);if(space!=std::u16string::npos&&space>pos)end=space;}out.push_back(src.substr(pos,end-pos));pos=end;while(pos<src.size()&&src[pos]==u' ')++pos;}return out;}
    static std::string toUtf8(const std::u16string& s){std::string out;for(char16_t c:s){if(c<0x80)out.push_back(static_cast<char>(c));else if(c<0x800){out.push_back(static_cast<char>(0xC0|(c>>6)));out.push_back(static_cast<char>(0x80|(c&0x3F)));}else{out.push_back(static_cast<char>(0xE0|(c>>12)));out.push_back(static_cast<char>(0x80|((c>>6)&0x3F)));out.push_back(static_cast<char>(0x80|(c&0x3F)));}}return out;}
    EditController* controller_{nullptr};VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};
static void drawButtonGlow(VSTGUI::CDrawContext* context,VSTGUI::CRect rect,const VSTGUI::CColor& color){rect.inset(2.,2.);VSTGUI::CColor fill=color;fill.alpha=58;context->setFillColor(fill);context->drawRect(rect,VSTGUI::kDrawFilled);VSTGUI::CColor edge=color;edge.alpha=245;context->setFrameColor(edge);context->setLineWidth(1.0);context->drawRect(rect,VSTGUI::kDrawStroked);auto h1=rect;h1.inset(1.,1.);VSTGUI::CColor c1=color;c1.alpha=150;context->setFrameColor(c1);context->drawRect(h1,VSTGUI::kDrawStroked);auto h2=rect;h2.inset(3.,3.);VSTGUI::CColor c2=color;c2.alpha=88;context->setFrameColor(c2);context->drawRect(h2,VSTGUI::kDrawStroked);auto h3=rect;h3.inset(5.,5.);VSTGUI::CColor c3=color;c3.alpha=42;context->setFrameColor(c3);context->drawRect(h3,VSTGUI::kDrawStroked);}
class ParameterButtonView final:public VSTGUI::CView{public:ParameterButtonView(const VSTGUI::CRect& size,EditController* controller,ParamID paramId,double value,const VSTGUI::CColor& glowColor):CView(size),controller_(controller),paramId_(paramId),value_(value),glowColor_(glowColor){setMouseEnabled(true);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}void draw(VSTGUI::CDrawContext* context)override{if(controller_&&std::abs(controller_->getParamNormalized(paramId_)-value_)<0.20)drawButtonGlow(context,getViewSize(),glowColor_);setDirty(false);}VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&)override{if(!controller_)return VSTGUI::kMouseEventNotHandled;controller_->beginEdit(paramId_);controller_->setParamNormalized(paramId_,value_);controller_->performEdit(paramId_,value_);controller_->endEdit(paramId_);invalid();return VSTGUI::kMouseEventHandled;}private:EditController* controller_{nullptr};ParamID paramId_{0};double value_{0.0};VSTGUI::CColor glowColor_;VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;};
}
MotivatoratorEditor::MotivatoratorEditor(EditController* controller):VST3Editor(controller,"view","Motivatorator.uidesc"),controller_(controller){}
VSTGUI::CView* MotivatoratorEditor::createView(const VSTGUI::UIAttributes& attributes,const VSTGUI::IUIDescription* description){if(const auto name=attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName)){if(*name=="CharacterView")return new CharacterView(VSTGUI::CRect(18.,82.,335.,352.),controller_);if(*name=="ModeTitleView")return new ModeTitleView(VSTGUI::CRect(100.,10.,304.,48.),controller_);if(*name=="PhraseView")return new PhraseView(VSTGUI::CRect(315.,90.,665.,330.),controller_);if(*name=="ModeMotivator")return new ParameterButtonView(VSTGUI::CRect(242.,368.,312.,409.),controller_,kModeId,0.0,VSTGUI::CColor(255,177,45,255));if(*name=="ModeDemotivator")return new ParameterButtonView(VSTGUI::CRect(315.,369.,399.,408.),controller_,kModeId,0.5,VSTGUI::CColor(230,56,36,255));if(*name=="ModeMixed")return new ParameterButtonView(VSTGUI::CRect(397.,367.,470.,410.),controller_,kModeId,1.0,VSTGUI::CColor(255,125,35,255));if(*name=="CharacterGnomi")return new ParameterButtonView(VSTGUI::CRect(500.,369.,574.,408.),controller_,kCharacterId,0.0,VSTGUI::CColor(255,156,38,255));if(*name=="CharacterRocky")return new ParameterButtonView(VSTGUI::CRect(579.,369.,653.,408.),controller_,kCharacterId,0.5,VSTGUI::CColor(255,156,38,255));if(*name=="CharacterDom")return new ParameterButtonView(VSTGUI::CRect(658.,369.,735.,408.),controller_,kCharacterId,1.0,VSTGUI::CColor(255,156,38,255));}return VSTGUI::VST3Editor::createView(attributes,description);}
} // namespace Steinberg::Vst
