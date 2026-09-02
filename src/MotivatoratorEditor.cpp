#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/uidescription/uiattributes.h"
#include <algorithm>
#include <cmath>

namespace Steinberg::Vst {

namespace {

class CharacterView final : public VSTGUI::CView {
public:
    CharacterView(const VSTGUI::CRect& size, EditController* controller)
    : CView(size), controller_(controller) {
        positive_ = VSTGUI::makeOwned<VSTGUI::CBitmap>(VSTGUI::CResourceDescription("gnomi_positive.png"));
        negative_ = VSTGUI::makeOwned<VSTGUI::CBitmap>(VSTGUI::CResourceDescription("gnomi_negative.png"));

        // The new GNOMI assets are square HiDPI cutouts. A 4x logical scale keeps
        // plenty of resolution for later GUI scaling while avoiding the old oversized look.
        if (positive_ && positive_->getPlatformBitmap())
            positive_->getPlatformBitmap()->setScaleFactor(4.0);
        if (negative_ && negative_->getPlatformBitmap())
            negative_->getPlatformBitmap()->setScaleFactor(4.0);

        setMouseEnabled(false);
        timer_ = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>(
            [this](VSTGUI::CVSTGUITimer*) { invalid(); }, 100);
    }

    void draw(VSTGUI::CDrawContext* context) override {
        if (!controller_) {
            setDirty(false);
            return;
        }

        const double modeValue = controller_->getParamNormalized(kModeId);
        const double toneValue = controller_->getParamNormalized(kPhraseToneId);
        const int mode = std::clamp(static_cast<int>(std::lround(modeValue * 2.0)), 0, 2);

        bool useNegative = false;
        if (mode == 1)
            useNegative = true;
        else if (mode == 2)
            useNegative = toneValue >= 0.5;

        auto bitmap = useNegative ? negative_ : positive_;
        if (bitmap) {
            // The replacement assets are composed as square character cutouts, so draw
            // them into a square 270x270 slot. This preserves their proportions and lets
            // the boots/body meet the studio surface naturally instead of being masked off.
            bitmap->draw(context, getViewSize(), VSTGUI::CPoint(0., 0.), 1.f);
        }
        setDirty(false);
    }

private:
    EditController* controller_ {nullptr};
    VSTGUI::SharedPointer<VSTGUI::CBitmap> positive_;
    VSTGUI::SharedPointer<VSTGUI::CBitmap> negative_;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class ModeHitView final : public VSTGUI::CView {
public:
    ModeHitView(const VSTGUI::CRect& size, EditController* controller, double value)
    : CView(size), controller_(controller), value_(value) {
        setMouseEnabled(true);
    }

    void draw(VSTGUI::CDrawContext*) override { setDirty(false); }

    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&, const VSTGUI::CButtonState&) override {
        if (!controller_)
            return VSTGUI::kMouseEventNotHandled;

        controller_->beginEdit(kModeId);
        controller_->setParamNormalized(kModeId, value_);
        controller_->performEdit(kModeId, value_);
        controller_->endEdit(kModeId);
        invalid();
        return VSTGUI::kMouseEventHandled;
    }

private:
    EditController* controller_ {nullptr};
    double value_ {0.0};
};

} // namespace

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc"), controller_(controller) {}

VSTGUI::CView* MotivatoratorEditor::createView(const VSTGUI::UIAttributes& attributes,
                                                const VSTGUI::IUIDescription* description) {
    if (const auto name = attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName)) {
        if (*name == "CharacterView")
            return new CharacterView(VSTGUI::CRect(18., 82., 288., 352.), controller_);
        if (*name == "ModeMotivator")
            return new ModeHitView(VSTGUI::CRect(241., 369., 310., 408.), controller_, 0.0);
        if (*name == "ModeDemotivator")
            return new ModeHitView(VSTGUI::CRect(315., 369., 399., 408.), controller_, 0.5);
        if (*name == "ModeMixed")
            return new ModeHitView(VSTGUI::CRect(404., 369., 470., 408.), controller_, 1.0);
    }
    return VSTGUI::VST3Editor::createView(attributes, description);
}

} // namespace Steinberg::Vst
