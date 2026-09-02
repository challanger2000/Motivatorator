#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cviewcontainer.h"

namespace Steinberg::Vst {

class MotivatoratorEditor final : public VSTGUI::VST3Editor {
public:
    explicit MotivatoratorEditor(EditController* controller);

protected:
    VSTGUI::CView* verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                             const VSTGUI::IUIDescription* description) override;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    void showOptions(bool show);

    VSTGUI::CViewContainer* optionsPanel_ {nullptr};
    bool optionsVisible_ {false};
    bool optionsButtonDown_ {false};
};

} // namespace Steinberg::Vst
