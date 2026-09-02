#include "MotivatoratorEditor.h"
#include "MotivatoratorProcessor.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cdrawcontext.h"
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

        // The uploaded cutouts are roughly 1290x1210 px. At 4.5x they render
        // about 289x270 logical pixels and fit the accepted GNOMI position.
        if (positive_ && positive_->getPlatformBitmap())
            positive_->getPlatformBitmap()->setScaleFactor(4.5);
        if (negative_ && negative_->getPlatformBitmap())
            negative_->getPlatformBitmap()->setScaleFactor(4.5);

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
        if (bitmap)
            bitmap->draw(context, getViewSize(), VSTGUI::CPoint(0., 0.), 1.f);

        setDirty(false);
    }

private:
    EditController* controller_ {nullptr};
    VSTGUI::SharedPointer<VSTGUI::CBitmap> positive_;
    VSTGUI::SharedPointer<VSTGUI::CBitmap> negative_;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

class ModeButtonView final : public VSTGUI::CView {
public:
    ModeButtonView(const VSTGUI::CRect& size, EditController* controller, double value,
                   const VSTGUI::CColor& glowColor)
    : CView(size), controller_(controller), value_(value), glowColor_(glowColor) {
        setMouseEnabled(true);
        timer_ = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>(
            [this](VSTGUI::CVSTGUITimer*) { invalid(); }, 100);
    }

    void draw(VSTGUI::CDrawContext* context) override {
        if (!controller_) {
            setDirty(false);
            return;
        }

        const double current = controller_->getParamNormalized(kModeId);
        const bool active = std::abs(current - value_) < 0.20;

        if (active) {
            auto r = getViewSize();
            r.inset(3., 4.);

            // Soft inner glow over the labels already painted into the background.
            VSTGUI::CColor soft = glowColor_;
            soft.alpha = 42;
            context->setFillColor(soft);
            context->drawRect(r, VSTGUI::kDrawFilled);

            // Bright border gives immediate, unambiguous active-state feedback.
            VSTGUI::CColor edge = glowColor_;
            edge.alpha = 220;
            context->setFrameColor(edge);
            context->setLineWidth(1.6);
            context->drawRect(r, VSTGUI::kDrawStroked);

            auto inner = r;
            inner.inset(2., 2.);
            VSTGUI::CColor innerEdge = glowColor_;
            innerEdge.alpha = 95;
            context->setFrameColor(innerEdge);
            context->setLineWidth(1.0);
            context->drawRect(inner, VSTGUI::kDrawStroked);
        }

        setDirty(false);
    }

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
    VSTGUI::CColor glowColor_;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
};

} // namespace

MotivatoratorEditor::MotivatoratorEditor(EditController* controller)
: VST3Editor(controller, "view", "Motivatorator.uidesc"), controller_(controller) {}

VSTGUI::CView* MotivatoratorEditor::createView(const VSTGUI::UIAttributes& attributes,
                                                const VSTGUI::IUIDescription* description) {
    if (const auto name = attributes.getAttributeValue(VSTGUI::IUIDescription::kCustomViewName)) {
        if (*name == "CharacterView")
            return new CharacterView(VSTGUI::CRect(18., 82., 335., 352.), controller_);
        if (*name == "ModeMotivator")
            return new ModeButtonView(VSTGUI::CRect(241., 369., 310., 408.), controller_, 0.0,
                                      VSTGUI::CColor(255, 177, 45, 255));
        if (*name == "ModeDemotivator")
            return new ModeButtonView(VSTGUI::CRect(315., 369., 399., 408.), controller_, 0.5,
                                      VSTGUI::CColor(230, 56, 36, 255));
        if (*name == "ModeMixed")
            return new ModeButtonView(VSTGUI::CRect(404., 369., 470., 408.), controller_, 1.0,
                                      VSTGUI::CColor(255, 125, 35, 255));
    }
    return VSTGUI::VST3Editor::createView(attributes, description);
}

} // namespace Steinberg::Vst
