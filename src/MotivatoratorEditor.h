#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"

namespace Steinberg::Vst {

class MotivatoratorEditor final : public VSTGUI::VST3Editor {
public:
    explicit MotivatoratorEditor(EditController* controller);

protected:
    VSTGUI::CView* createView(const VSTGUI::UIAttributes& attributes,
                              const VSTGUI::IUIDescription* description) override;

private:
    EditController* controller_ {nullptr};
};

} // namespace Steinberg::Vst
