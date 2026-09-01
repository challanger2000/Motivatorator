#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/uidescription/uiattributes.h"

namespace Steinberg::Vst {

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc") {}

VSTGUI::CView* MotivatoratorEditor::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                                               const VSTGUI::IUIDescription* description) {
    std::string customName;
    if (attributes.getAttributeValue("custom-view-name", customName) && customName == "OptionsPanel") {
        if (auto* container = dynamic_cast<VSTGUI::CViewContainer*>(view)) {
            optionsPanel_ = container;
            optionsPanel_->setVisible(false);
            optionsPanel_->setMouseEnabled(false);
        }
    }
    return VST3Editor::verifyView(view, attributes, description);
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

    if (control->getTag() == static_cast<int32>(kOptionsId)) {
        const bool down = control->getValue() >= 0.5f;
        if (down && !optionsButtonDown_) showOptions(!optionsVisible_);
        optionsButtonDown_ = down;
        return;
    }

    VST3Editor::valueChanged(control);
}

} // namespace Steinberg::Vst
