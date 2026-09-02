#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/uidescription/uiattributes.h"
#include <algorithm>
#include <cmath>

namespace Steinberg::Vst {
namespace {
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
static void drawButtonGlow(VSTGUI::CDrawContext* context,VSTGUI::CRect rect,const VSTGUI::CColor& color){
    rect.inset(2.,2.); VSTGUI::CColor fill=color; fill.alpha=30; context->setFillColor(fill); context->drawRect(rect,VSTGUI::kDrawFilled);
    VSTGUI::CColor edge=color; edge.alpha=210; context->setFrameColor(edge); context->setLineWidth(1.0); context->drawRect(rect,VSTGUI::kDrawStroked);
    auto inner=rect; inner.inset(2.,2.); VSTGUI::CColor innerEdge=color; innerEdge.alpha=64; context->setFrameColor(innerEdge); context->drawRect(inner,VSTGUI::kDrawStroked);
}
class ParameterButtonView final : public VSTGUI::CView {
public:
    ParameterButtonView(const VSTGUI::CRect& size,EditController* controller,ParamID paramId,double value,const VSTGUI::CColor& glowColor)
    :CView(size),controller_(controller),paramId_(paramId),value_(value),glowColor_(glowColor){setMouseEnabled(true);timer_=VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>([this](VSTGUI::CVSTGUITimer*){invalid();},100);}
    void draw(VSTGUI::CDrawContext* context) override {if(controller_&&std::abs(controller_->getParamNormalized(paramId_)-value_)<0.20) drawButtonGlow(context,getViewSize(),glowColor_);setDirty(false);}
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override {if(!controller_)return VSTGUI::kMouseEventNotHandled;controller_->beginEdit(paramId_);controller_->setParamNormalized(paramId_,value_);controller_->performEdit(paramId_,value_);controller_->endEdit(paramId_);invalid();return VSTGUI::kMouseEventHandled;}
private: EditController* controller_{nullptr}; ParamID paramId_{0}; double value_{0.0}; VSTGUI::CColor glowColor_; VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};
} // namespace
MotivatoratorEditor::MotivatoratorEditor(EditController* controller):VST3Editor(controller,"view","Motivatorator.uidesc"),controller_(controller){}
VSTGUI::CView* MotivatoratorEditor::createView(const VSTGUI::UIAttributes& attributes,const VSTGUI::IUIDescription* description){
    if(const auto name=attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName)){
        if(*name=="CharacterView") return new CharacterView(VSTGUI::CRect(18.,82.,335.,352.),controller_);
        if(*name=="ModeMotivator") return new ParameterButtonView(VSTGUI::CRect(242.,368.,312.,409.),controller_,kModeId,0.0,VSTGUI::CColor(255,177,45,255));
        if(*name=="ModeDemotivator") return new ParameterButtonView(VSTGUI::CRect(315.,369.,399.,408.),controller_,kModeId,0.5,VSTGUI::CColor(230,56,36,255));
        // The glow function already insets by 2 px. Give MIXED the full baked-button bounds here
        // instead of shrinking the view itself, so the visible glow lands on the actual button face.
        if(*name=="ModeMixed") return new ParameterButtonView(VSTGUI::CRect(397.,367.,470.,410.),controller_,kModeId,1.0,VSTGUI::CColor(255,125,35,255));
        if(*name=="CharacterGnomi") return new ParameterButtonView(VSTGUI::CRect(500.,369.,574.,408.),controller_,kCharacterId,0.0,VSTGUI::CColor(255,156,38,255));
        if(*name=="CharacterRocky") return new ParameterButtonView(VSTGUI::CRect(579.,369.,653.,408.),controller_,kCharacterId,0.5,VSTGUI::CColor(255,156,38,255));
        if(*name=="CharacterDom") return new ParameterButtonView(VSTGUI::CRect(658.,369.,735.,408.),controller_,kCharacterId,1.0,VSTGUI::CColor(255,156,38,255));
    }
    return VSTGUI::VST3Editor::createView(attributes,description);
}
} // namespace Steinberg::Vst
