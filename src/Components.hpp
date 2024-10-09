#pragma once

#include <rack.hpp>

using namespace rack;

namespace ph {

template <typename TBase = GrayModuleLightWidget>
struct ConnectionLight : TBase {
    ConnectionLight(Vec p0, Vec p1) {
        this->angle = atan2(p1.y - p0.y, p1.x - p0.x);

        // Calculate new endpoint on the edge of the button
        p0 = p0.plus(14 * Vec(cos(angle), sin(angle)));
        p1 = p1.minus(14 * Vec(cos(angle), sin(angle)));

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
    }

    void drawBackground(const Widget::DrawArgs& args) override {
        // nvgBeginPath(args.vg);
        // nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);
        // nvgStrokeColor(args.vg, nvgRGB(0, 0, 0));
        // nvgStrokeWidth(args.vg, 1.0);
        // nvgStroke(args.vg);
    }

    void drawLight(const Widget::DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, start.x, start.y);
        nvgLineTo(args.vg, end.x, end.y);
        nvgStrokeColor(args.vg, this->color);
        nvgStrokeWidth(args.vg, 2.0); // TODO: magic number
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, indicatorPos.x, indicatorPos.y, 2.0f); // TODO: magic number
        nvgFillColor(args.vg, this->color);
        nvgAlpha(args.vg, alpha);
        nvgFill(args.vg);
    }

    void drawHalo(const Widget::DrawArgs& args) override {
        // TODO
    }

    void step() override {
        animTime += 0.01f;
        if (animTime >= 1.0f) {
            animTime = 0.0f;
        }

        indicatorPos = (flipped ? (1.0f - animTime) : animTime) * end;
        alpha = std::sin(M_PI * animTime);
        TBase::step();
    }

private:
    bool flipped = false;
    Vec start = {0, 0};
    Vec end;
    float animTime = 0.0f;
    Vec indicatorPos = {0, 0};

    float alpha = 1.f;
    float angle;
};

template <typename TBase = GreenLight>
ConnectionLight<TBase>* createConnectionLight(Vec p0, Vec p1, Module* module, int firstLightId) {
    auto* light = new ConnectionLight<TBase>(p0, p1);
    light->module = module;
    light->firstLightId = firstLightId;
    return light;
}

} // namespace ph