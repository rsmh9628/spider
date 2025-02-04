#pragma once

#include <rack.hpp>
#include "Colour.hpp"
#include "plugin.hpp"
#include <functional>

using namespace rack;

namespace ph {

struct ConnectionLight : ModuleLightWidget {
    ConnectionLight(Vec p0, Vec p1, int op)
        : op(op) {
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
        NVGcolor strokeColour = nvgRGB(30, 30, 30);
        
        drawDottedLine(args.vg, [strokeColour](NVGcontext* vg, int, int, Vec pos) {
            nvgBeginPath(vg);
            nvgCircle(vg, pos.x, pos.y, 1.5f);

            nvgFillColor(vg, strokeColour);
            nvgFill(vg);
        });
    }

    void drawLight(const Widget::DrawArgs& args) override {
        drawDottedLine(args.vg, [this](NVGcontext* vg, int i, int numDots, Vec pos) {
            nvgBeginPath(vg);
            nvgCircle(vg, pos.x, pos.y, 1.5f);

            float dotAlpha = 1.f;
            float t;
            if (flipped) {
                t = animTime + i * 0.1f;
            } else {
                t = animTime + (numDots - i) * 0.1f;
            }
            dotAlpha = 0.5f + 0.5f * std::sin(t * 2.0f * M_PI);

            auto segmentColour = OPERATOR_COLOURS[op];
            segmentColour.a = getLight(0)->getBrightness() * dotAlpha;

            nvgFillColor(vg, segmentColour);
            nvgFill(vg);
        });
        
        //drawDottedLine(args.vg, start, end, OPERATOR_COLOURS[op], getLight(0)->getBrightness(), true);
    }

    void drawHalo(const Widget::DrawArgs& args) override {
        if (args.fb) 
            return;
        
        if (rack::settings::haloBrightness <= 0.0f)
            return;
        
        drawDottedLine(args.vg, [this](NVGcontext* vg, int i, int numDots, Vec pos) {
            nvgBeginPath(vg);
            nvgCircle(vg, pos.x, pos.y, 8.0f);

            float dotAlpha = 1.f;
            float t;
            if (flipped) {
                t = animTime + i * 0.1f;
            } else {
                t = animTime + (numDots - i) * 0.1f;
            }
            dotAlpha = 0.5f + 0.5f * std::sin(t * 2.0f * M_PI);

            auto segmentColour = OPERATOR_COLOURS[op];
            segmentColour.a = getLight(0)->getBrightness() * dotAlpha * rack::settings::haloBrightness;

            auto ocol = nvgRGBAf(0.0f, 0.0f, 0.0f, 0.0f);
            NVGpaint paint = nvgRadialGradient(vg, pos.x, pos.y, 2.0f, 8.0f, segmentColour, nvgRGBAf(0.f, 0.f, 0.f, 0.f));
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        });
    }

    std::vector<Vec> calculateDotPositions() {
        const float dotSpacing = 7.0f;

        std::vector<Vec> positions;
        float distance = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
        int numDots = static_cast<int>(distance / dotSpacing); 
        float dx = (end.x - start.x) / numDots;
        float dy = (end.y - start.y) / numDots;

        for (int i = 0; i <= numDots; ++i) {
            float x = start.x + i * dx;
            float y = start.y + i * dy;
            positions.push_back(Vec(x, y));
        }

        return positions;
    }

    void drawDottedLine(NVGcontext* vg, std::function<void(NVGcontext*, int, int, Vec)> drawFunc) {
        const float dotSpacing = 7.0f;

        std::vector<Vec> positions;
        float distance = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
        int numDots = static_cast<int>(distance / dotSpacing); 
        float dx = (end.x - start.x) / numDots;
        float dy = (end.y - start.y) / numDots;

        for (int i = 0; i <= numDots; ++i) {
            float x = start.x + i * dx;
            float y = start.y + i * dy;

            drawFunc(vg, i, numDots, Vec(x, y));
            //nvgBeginPath(vg);
            //nvgCircle(vg, x, y, 2.0f); // Adjust the radius of the dots as needed
            //
            //float segmentAlpha = 1.f;
//
            //if (animated) {
            //    float t;
            //    if (flipped) {
            //        t = animTime + i * 0.1f;
            //    } else {
            //        t = animTime + (numDots - i) * 0.1f;
            //    }
            //    segmentAlpha = 0.5f + 0.5f * std::sin(t * 2.0f * M_PI);
            //} else {
            //    segmentAlpha = 1.0f;
            //}
//
            //auto segmentColour = color;
            //segmentColour.a = alpha * segmentAlpha;
//
            //nvgFillColor(vg, segmentColour);
            //nvgFill(vg);
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
    int op = 0;
    float animTime = 0.0f;
    Vec indicatorPos = {0, 0};
    float indicatorAlpha = 1.0f;
    float indicatorRadius;

    float alpha = 1.f;
    float angle;
    float length;
};

ConnectionLight* createConnectionLight(Vec p0, Vec p1, Module* module, int firstLightId, int op) {
    auto* light = new ConnectionLight(p0, p1, op);
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

struct HexLight : GrayModuleLightWidget {
    HexLight(Vec pos, int op) {
        this->box.pos = pos - Vec(radius, radius);
        this->addBaseColor(OPERATOR_COLOURS[op]);
        this->addBaseColor(nvgRGB(255, 255, 255));
        this->box.size = Vec(20, 20);
    }

    void drawBackground(const Widget::DrawArgs& args) override {
        //nvgBeginPath(args.vg);
//
        //for (int i = 0; i < 6; i++) {
        //    float angle = M_PI / 3.0 * i;
        //    float x = radius + radius * cos(angle);
        //    float y = radius + radius * sin(angle);
        //    if (i == 0) {
        //        nvgMoveTo(args.vg, x, y);
        //    } else {
        //        nvgLineTo(args.vg, x, y);
        //    }
        //}
//
        //nvgStrokeColor(args.vg, nvgRGB(50, 50, 50));
        //nvgStrokeWidth(args.vg, 1.f);
        //nvgStroke(args.vg);
    }

    void drawLight(const Widget::DrawArgs& args) override {
        
        if (color.a > 0.0) {
            drawHexagon(args);

            nvgStrokeColor(args.vg, color);
            nvgStrokeWidth(args.vg, 1.f);
            nvgStroke(args.vg);
        }
    }

    void drawHexagon(const Widget::DrawArgs& args) {
        nvgBeginPath(args.vg);
        for (int i = 0; i < 6; i++) {
            float angle = M_PI / 3.0 * i;
            float x = radius + radius * cos(angle);
            float y = radius + radius * sin(angle);
            if (i == 0) {
                nvgMoveTo(args.vg, x, y);
            } else {
                nvgLineTo(args.vg, x, y);
            }
        }
        nvgClosePath(args.vg);
    }

    float radius = 10.f;
};

HexLight* createHexLight(Vec pos, Module* module, int firstLightId, int op) {
    auto* light = new HexLight(pos, op);
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