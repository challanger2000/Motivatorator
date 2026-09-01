#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/controls/ccontrol.h"

namespace Steinberg::Vst {

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc") {
    setController(this);
}

VSTGUI::CView* MotivatoratorEditor::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                                               const VSTGUI::IUIDescription*) {
    std::string customName;
    if (attributes.getAttributeValue("custom-view-name", customName) && customName == "OptionsPanel") {
        if (auto* container = dynamic_cast<VSTGUI::CViewContainer*>(view)) {
            optionsPanel_ = container;
            optionsPanel_->setVisible(false);
            optionsPanel_->setMouseEnabled(false);
        }
    }
    return view;
}

VSTGUI::IControlListener* MotivatoratorEditor::getControlListener(VSTGUI::UTF8StringPtr name) {
    if (name && std::string(name) == "OptionsListener") return this;
    return nullptr;
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
    showOptions(!optionsVisible_);
}

} // namespace Steinberg::Vst
