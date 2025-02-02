#pragma once

#include <rack.hpp>
#include "Colour.hpp"
#include "plugin.hpp"

using namespace rack;

#define DEFINE_OP_LIGHT(N)                           \
    \ 
    struct TOp##N##Light : GrayModuleLightWidget {   \
        TOp##N##Light() {                            \
            this->addBaseColor(OPERATOR_COLOURS[N]); \
        }                                            \
    };                                               \
    using Op##N##Light = TOp##N##Light;

namespace ph {

struct ConnectionLight : ModuleLightWidget {
    ConnectionLight(Vec p0, Vec p1, int op0, int op1)
        : op0(op0)
        , op1(op1) {
        this->angle = atan2(p1.y - p0.y, p1.x - p0.x);

        // Calculate new endpoint on the edge of the button
        p0 = p0.plus(15 * Vec(cos(angle), sin(angle)));
        p1 = p1.minus(15 * Vec(cos(angle), sin(angle)));

        if (p0.x < p1.x || (p0.x == p1.x && p0.y < p1.y)) {
            start = p0;
            end = p1;
        } else {
            start = p1;
            end = p0;
            flipped = true;
        }

        this->box.pos = start;
        this->box.size = end.minus(start);

        start = Vec(0.0f, 0.0f);
        end = Vec(this->box.size.x, this->box.size.y);

        length = std::hypot(this->box.size.x, this->box.size.y);
    }

    void drawBackground(const Widget::DrawArgs& args) override {
        NVGcolor strokeColour = nvgRGB(20, 20, 20);
        drawSegmentedLine(args.vg, start, end, strokeColour, 1.f, false);
    }

    void drawLight(const Widget::DrawArgs& args) override {
        drawSegmentedLine(args.vg, start, end, OPERATOR_COLOURS[op0], getLight(0)->getBrightness(), true);
    }

    void drawSegmentedLine(NVGcontext* vg, const Vec& start, const Vec& end, NVGcolor color, float alpha,
                           bool animated) {
        nvgStrokeWidth(vg, 1.f);    // TODO: magic number

        Vec p0 = start;
        Vec p1 = end;

        Vec direction = p1.minus(p0).normalize();
        float totalLength = sqrt(pow(p1.x - p0.x, 2) + pow(p1.y - p0.y, 2));
        float gap = 7.0f; // Fixed segment gap
        int numSegments = static_cast<int>(totalLength / gap);

        for (int i = 0; i < numSegments; ++i) {
            Vec segmentStart = p0.plus(direction.mult(gap * i));
            float segmentAlpha = 1.f;

            if (animated) {
                if (animTime < 0.5f) {
                    segmentAlpha = std::min(1.0f, std::max(0.0f, animTime * 2 * numSegments - (flipped ? (numSegments - 1 - i) : i)));
                } else {
                    segmentAlpha = std::min(1.0f, std::max(0.0f, (1.0f - animTime) * 2 * numSegments - (flipped ? i : (numSegments - 1 - i))));
                }
            } else {
                segmentAlpha = 1.0f;
            }

            auto segmentColour = color;
            segmentColour.a = alpha * segmentAlpha;

            nvgBeginPath(vg);
            nvgCircle(vg, segmentStart.x, segmentStart.y, 2.0f); // Draw circle with radius 2.0f
            nvgFillColor(vg, segmentColour);
            nvgFill(vg);
        }
    }

    void drawArrowhead(NVGcontext* vg, float x0, float y0, float x1, float y1, NVGcolor colour) {
        const float arrowSize = 5.0f; // Size of the arrowhead
        float angle = atan2(y1 - y0, x1 - x0);

        float arrowX1 = x0 + arrowSize * cos(angle + M_PI / 6);
        float arrowY1 = y0 + arrowSize * sin(angle + M_PI / 6);
        float arrowX2 = x0 + arrowSize * cos(angle - M_PI / 6);
        float arrowY2 = y0 + arrowSize * sin(angle - M_PI / 6);

        nvgBeginPath(vg);
        nvgMoveTo(vg, x0, y0);
        nvgLineTo(vg, arrowX1, arrowY1);
        nvgLineTo(vg, arrowX2, arrowY2);
        nvgClosePath(vg);

        nvgStrokeColor(vg, colour);
        nvgStroke(vg);
    }

    void drawHalo(const Widget::DrawArgs& args) override {
        //// Draw the glow around the light
        //float radius = 5.0f;
//
        //float x = start.x - radius;
        //float y = start.y - radius;
        //float w = length + radius * 2;
        //float h = radius + radius;
//
        //float haloBrightness = rack::settings::haloBrightness;
//
        //if (flipped) {
        //    // nvgTranslate(args.vg, end.x, end.y);
        //    nvgRotate(args.vg, angle + M_PI);
        //} else {
        //    nvgRotate(args.vg, angle);
        //}
//
        //nvgBeginPath(args.vg);
//
        //nvgRoundedRect(args.vg, x - w, y - h, w * 3.f, h * 3.f, h * 1.0f);
        //nvgFillPaint(args.vg, nvgBoxGradient(args.vg, x, y, w, h, 0.0f, h * 0.5f,
        //                                     rack::color::mult(OPERATOR_COLOURS[op0],
        //                                                       getLight(0)->getBrightness() * haloBrightness),
        //                                     nvgRGBA(0, 0, 0, 0)));
        //nvgFill(args.vg);
//
        // float indicatorX = indicatorPos.x - radius;
        // float indicatorY = indicatorPos.y - radius;
        //
        // float indicatorW = radius + radius;
        // float indicatorH = radius + radius;
        //
        // nvgBeginPath(args.vg);
        // nvgRoundedRect(args.vg, indicatorX - indicatorW, indicatorY - indicatorH, w * 3.f, h * 3.f, h * 1.0f);
        // nvgFillPaint(args.vg, nvgRadialGradient(args.vg, indicatorPos.x, indicatorPos.y, 0.0f, indicatorRadius,
        //                                          nvgRGB(255, 255, 255), nvgRGB(0, 0, 0)));
        //
        // nvgFill(args.vg);

        // nvgFillColor(args.vg, nvgRGB(255, 255, 255));
    }

    void step() override {
        if (trigger.process(getLight(0)->getBrightness())) {
            animTime = 0.0f;
        }

        if (getLight(0)->getBrightness() > 0.0f) {
            animTime += 0.01f;
            if (animTime >= 1.0f) {
                animTime = 0.0f;
            }
        }

        ModuleLightWidget::step();
    }

    // Tooltips are not useful for this widget
    void onEnter(const EnterEvent& e) override { return; }
    void onLeave(const LeaveEvent& e) override { return; }

private:
    dsp::BooleanTrigger trigger;

    bool flipped = false;
    Vec start = {0, 0};
    Vec end;
    int op0 = 0;
    int op1 = 0;
    float animTime = 0.0f;
    Vec indicatorPos = {0, 0};
    float indicatorAlpha = 1.0f;
    float indicatorRadius;

    float alpha = 1.f;
    float angle;
    float length;
};
DEFINE_OP_LIGHT(0)
DEFINE_OP_LIGHT(1)
DEFINE_OP_LIGHT(2)
DEFINE_OP_LIGHT(3)
DEFINE_OP_LIGHT(4)
DEFINE_OP_LIGHT(5)

ConnectionLight* createConnectionLight(Vec p0, Vec p1, Module* module, int firstLightId, int op0, int op1) {
    auto* light = new ConnectionLight(p0, p1, op0, op1);
    light->module = module;
    light->firstLightId = firstLightId;
    return light;
}

struct HexJack : SvgPort {
    HexJack() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/HexJack.svg")));
    }
};

struct ShinyKnob : RoundKnob {
    ShinyKnob() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ShinyKnob.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/ShinyKnob_bg.svg")));
    }
};

struct ShinyBigKnob : RoundKnob {
    ShinyBigKnob() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ShinyBigKnob.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/ShinyBigKnob_bg.svg")));
    }
};

struct AttenuatorKnob : Trimpot {
    AttenuatorKnob() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/AttenuatorKnob.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/AttenuatorKnob_bg.svg")));
    }
};

struct RingLight : GrayModuleLightWidget {
    RingLight(Vec pos, int op) {
        this->box.pos = pos - Vec(radius, radius);
        this->addBaseColor(OPERATOR_COLOURS[op]);
        this->box.size = Vec(20, 20);
    }

    void drawBackground(const Widget::DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, radius, radius, radius);

        nvgStrokeColor(args.vg, nvgRGB(50, 50, 50));
        nvgStrokeWidth(args.vg, 1.f);
        nvgStroke(args.vg);
    }

    void drawLight(const Widget::DrawArgs& args) override {
        if (color.a > 0.0) {

            nvgBeginPath(args.vg);
            nvgCircle(args.vg, radius, radius, radius);

            nvgStrokeColor(args.vg, color);
            nvgStrokeWidth(args.vg, 1.f);
            nvgStroke(args.vg);
        }
    }

    float radius = 10.f;
};

RingLight* createRingLight(Vec pos, Module* module, int firstLightId, int op) {
    auto* light = new RingLight(pos, op);
    light->module = module;
    light->firstLightId = firstLightId;
    return light;
}

struct OperatorButton : app::SvgSwitch {
	OperatorButton() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/Operator1_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/Operator1_1.svg")));
	}
};

} // namespace ph