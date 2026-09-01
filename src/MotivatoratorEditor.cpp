#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/ccontrol.h"

namespace Steinberg::Vst {

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc") {
    setController(this);
}

VSTGUI::CView* MotivatoratorEditor::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                                               const VSTGUI::IUIDescription* description) {
    if (auto* container = dynamic_cast<VSTGUI::CViewContainer*>(view)) {
        VSTGUI::UTF8StringPtr customName = nullptr;
        if (attributes.getAttributeValue("custom-view-name", customName) && customName && std::string(customName) == "OptionsPanel") {
            optionsPanel_ = container;
            optionsPanel_->setVisible(false);
            optionsPanel_->setMouseEnabled(false);
        }
    }
    return view;
}

VSTGUI::IControlListener* MotivatoratorEditor::getControlListener(VSTGUI::UTF8StringPtr) {
    return this;
}

void MotivatoratorEditor::showOptions(bool show) {
    optionsVisible_ = show;
    if (optionsPanel_) {
        optionsPanel_->setVisible(show);
        optionsPanel_->setMouseEnabled(show);
        optionsPanel_->invalid();
    }
}

void MotivatoratorEditor::valueChanged(VSTGUI::CControl* control) {
    if (!control) return;
    if (control->getTag() == static_cast<int32>(kOptionsId) && control->getValue() >= 0.5f) {
        showOptions(!optionsVisible_);
        control->setValue(0.f);
        control->invalid();
    }
}

} // namespace Steinberg::Vst
