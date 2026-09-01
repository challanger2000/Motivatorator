#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/uidescription/icontroller.h"
#include "vstgui/lib/cviewcontainer.h"

namespace Steinberg::Vst {

class MotivatoratorEditor final : public VSTGUI::VST3Editor, public VSTGUI::IController {
public:
    explicit MotivatoratorEditor(EditController* controller);

    VSTGUI::CView* verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                             const VSTGUI::IUIDescription* description) override;
    VSTGUI::IControlListener* getControlListener(VSTGUI::UTF8StringPtr name) override;
    void valueChanged(VSTGUI::CControl* control) override;

private:
    void showOptions(bool show);
    VSTGUI::CViewContainer* optionsPanel_ {nullptr};
    bool optionsVisible_ {false};
};

} // namespace Steinberg::Vst
