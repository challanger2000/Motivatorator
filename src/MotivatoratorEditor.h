#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"

namespace Steinberg::Vst {

class MotivatoratorEditor final : public VSTGUI::VST3Editor {
public:
    explicit MotivatoratorEditor(EditController* controller);
};

} // namespace Steinberg::Vst
