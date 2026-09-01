#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/cparamdisplay.h"
#include "vstgui/lib/cvstguitimer.h"

namespace Steinberg::Vst {

class MotivatoratorEditor final : public VSTGUI::VST3Editor {
public:
    explicit MotivatoratorEditor(EditController* controller);
    ~MotivatoratorEditor() override;

protected:
    VSTGUI::CView* verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                             const VSTGUI::IUIDescription* description) override;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    void showOptions(bool show);
    void tickTicker();
    void resetTicker();

    VSTGUI::CViewContainer* optionsPanel_ {nullptr};
    VSTGUI::CParamDisplay* phraseTicker_ {nullptr};
    VSTGUI::CVSTGUITimer* tickerTimer_ {nullptr};
    float lastPhraseValue_ {-1.f};
    bool optionsVisible_ {false};
    bool optionsButtonDown_ {false};
};

} // namespace Steinberg::Vst
