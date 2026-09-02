#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "PhraseBank.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/uidescription/uiattributes.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Steinberg::Vst {

namespace {
using namespace MotivatoratorPhrases;

const TChar* phraseForGlobalIndex(int index) {
    const int motivatorCount = static_cast<int>(kMotivatorCount);
    const int demotivatorCount = static_cast<int>(kDemotivatorCount);
    const int phraseCount = static_cast<int>(kPhraseCount);

    if (index < motivatorCount) return kMotivator[static_cast<std::size_t>(index)].de;
    if (index < phraseCount) return kDemotivator[static_cast<std::size_t>(index - motivatorCount)].de;
    index -= phraseCount;
    if (index < motivatorCount) return kMotivator[static_cast<std::size_t>(index)].en;
    return kDemotivator[static_cast<std::size_t>(std::clamp(index - motivatorCount, 0, demotivatorCount - 1))].en;
}

std::size_t textLength(const TChar* text) {
    std::size_t length = 0;
    if (!text) return length;
    while (text[length] != 0) ++length;
    return length;
}
}

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc") {}

MotivatoratorEditor::~MotivatoratorEditor() {
    if (tickerTimer_) {
        tickerTimer_->stop();
        tickerTimer_->forget();
        tickerTimer_ = nullptr;
    }
}

VSTGUI::CView* MotivatoratorEditor::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                                               const VSTGUI::IUIDescription* description) {
    if (const auto* customName = attributes.getAttributeValue("custom-view-name")) {
        if (*customName == "OptionsPanel") {
            if (auto* container = dynamic_cast<VSTGUI::CViewContainer*>(view)) {
                optionsPanel_ = container;
                optionsPanel_->setVisible(false);
                optionsPanel_->setMouseEnabled(false);
            }
        } else if (*customName == "PhraseTicker") {
            if (auto* display = dynamic_cast<VSTGUI::CParamDisplay*>(view)) {
                phraseTicker_ = display;
                phraseTicker_->setMouseEnabled(false);
                phraseTicker_->setTransparency(true);
                lastPhraseValue_ = -1.f;
                if (!tickerTimer_)
                    tickerTimer_ = new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer*) { tickTicker(); }, 40, true);
            }
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

void MotivatoratorEditor::resetTicker() {
    if (!phraseTicker_) return;
    auto* parent = phraseTicker_->getParentView();
    if (!parent) return;

    const float value = phraseTicker_->getValue();
    const int total = static_cast<int>(MotivatoratorPhrases::kPhraseCount * 2);
    const int index = std::clamp(static_cast<int>(std::lround(value * static_cast<float>(total - 1))), 0, total - 1);
    const auto chars = textLength(phraseForGlobalIndex(index));
    const VSTGUI::CCoord estimatedWidth = std::max<VSTGUI::CCoord>(240.0, static_cast<VSTGUI::CCoord>(chars) * 11.5 + 80.0);

    const VSTGUI::CCoord viewportWidth = parent->getWidth();
    const VSTGUI::CCoord viewportHeight = parent->getHeight();
    VSTGUI::CRect r(viewportWidth, 0.0, viewportWidth + estimatedWidth, viewportHeight);
    phraseTicker_->setViewSize(r);
    phraseTicker_->setMouseableArea(r);
    phraseTicker_->invalid();
    lastPhraseValue_ = value;
}

void MotivatoratorEditor::tickTicker() {
    if (!phraseTicker_ || optionsVisible_) return;
    auto* parent = phraseTicker_->getParentView();
    if (!parent || !phraseTicker_->isVisible()) return;

    const float value = phraseTicker_->getValue();
    if (lastPhraseValue_ < 0.f || std::abs(value - lastPhraseValue_) > 0.00001f) {
        resetTicker();
        return;
    }

    auto oldRect = phraseTicker_->getViewSize();
    auto newRect = oldRect;
    newRect.offset(-2.0, 0.0);
    if (newRect.right < 0.0) {
        resetTicker();
        return;
    }

    phraseTicker_->setViewSize(newRect);
    phraseTicker_->setMouseableArea(newRect);

    // Only invalidate the small CRT viewport. Invalidating the moving child itself can
    // propagate its oversized scrolling rectangle outside the intended clip area in some hosts.
    parent->invalidRect(VSTGUI::CRect(0.0, 0.0, parent->getWidth(), parent->getHeight()));
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
