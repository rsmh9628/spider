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
        p0 = p0.plus(10 * Vec(cos(angle), sin(angle)));
        p1 = p1.minus(10 * Vec(cos(angle), sin(angle)));

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

        indicatorRadius = std::max(this->box.size.x / 2, this->box.size.y / 2);
    }

    void drawBackground(const Widget::DrawArgs& args) override {
        nvgBeginPath(args.vg);

        nvgAlpha(args.vg, 1.0f);

        NVGcolor strokeColour = nvgRGB(50, 50, 50);
        if (getLight(0)->getBrightness() > 0) {
            strokeColour = nvgRGB(0, 0, 0);
        }
        nvgStrokeColor(args.vg, strokeColour);

        nvgMoveTo(args.vg, start.x, start.y);
        nvgLineTo(args.vg, end.x, end.y);

        nvgStrokeWidth(args.vg, 1.0f); // TODO: magic number
        nvgStroke(args.vg);
    }

    void drawLight(const Widget::DrawArgs& args) override {
        nvgBeginPath(args.vg);

        nvgStrokeWidth(args.vg, 1.f); // TODO: magic number

        nvgMoveTo(args.vg, start.x, start.y);
        nvgLineTo(args.vg, end.x, end.y);

        nvgAlpha(args.vg, getLight(0)->getBrightness());

        nvgStrokeColor(args.vg, OPERATOR_COLOURS[op0]);
        nvgStroke(args.vg);

        nvgStrokePaint(args.vg, nvgRadialGradient(args.vg, indicatorPos.x, indicatorPos.y, 0.0f, indicatorRadius,
                                                  nvgRGB(255, 255, 255), nvgRGB(0, 0, 0)));
        nvgStroke(args.vg);
    }

    void drawHalo(const Widget::DrawArgs& args) override {
        // Draw the glow around the light
        float radius = 5.0f;

        float x = start.x - radius;
        float y = start.y - radius;
        float w = length + radius * 2;
        float h = radius + radius;

        float haloBrightness = rack::settings::haloBrightness;

        if (flipped) {
            // nvgTranslate(args.vg, end.x, end.y);
            nvgRotate(args.vg, angle + M_PI);
        } else {
            nvgRotate(args.vg, angle);
        }

        nvgBeginPath(args.vg);

        nvgRoundedRect(args.vg, x - w, y - h, w * 3.f, h * 3.f, h * 1.0f);
        nvgFillPaint(args.vg,
                     nvgBoxGradient(args.vg, x, y, w, h, 0.0f, h * 0.5f,
                                    rack::color::mult(OPERATOR_COLOURS[op0], haloBrightness), nvgRGBA(0, 0, 0, 0)));
        nvgFill(args.vg);

        float indicatorWidth = w / 2;
        if (x < 0) {
            indicatorWidth -= x;
        } else if (x > w) {
            indicatorWidth -= w - x;
        }

        nvgFillPaint(args.vg,
                     nvgBoxGradient(args.vg, indicatorPos.x - radius, indicatorPos.y - radius, indicatorWidth, h, 5.0f,
                                    20.0f, rack::color::mult(nvgRGB(255, 255, 255), haloBrightness * indicatorAlpha),
                                    nvgRGB(0, 0, 0)));
        nvgFill(args.vg);

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

    // Tooltips are not useful for this widget
    void onEnter(const EnterEvent& e) override { return; }
    void onLeave(const LeaveEvent& e) override { return; }

    void step() override {
        if (trigger.process(getLight(0)->getBrightness())) {
            animTime = 0.0f;
            enabled = true;
        }

        if (getLight(0)->getBrightness() > 0.0f) {
            animTime += 0.01f;
            if (animTime >= 1.0f) {
                animTime = 0.0f;
            }

            float position = (flipped) ? 1.5f - animTime * 2.0f : animTime * 2.0f - 0.5f;

            // Calculate indicatorAlpha based on position
            if (position <= 0.5f) {
                indicatorAlpha = position * 2.0f; // Smoothly transition from 0 to 1
            } else {
                indicatorAlpha = (1.0f - position) * 2.0f; // Smoothly transition from 1 to 0
            }

            indicatorPos = end * position;
        }

        ModuleLightWidget::step();
    }

private:
    dsp::BooleanTrigger trigger;
    bool enabled = false;

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

} // namespace ph