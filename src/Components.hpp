#pragma once

#include <rack.hpp>

using namespace rack;

namespace ph {

template <typename TBase = ModuleLightWidget>
struct ConnectionLight : TBase {
    ConnectionLight(Vec p0, Vec p1)
        : p0(p0)
        , p1(p1) {
        this->box.size = Vec(std::fabs(p1.x - p0.x), std::fabs(p1.y - p0.y));

        this->angle = atan2(p1.y - p0.y, p1.x - p0.x);
        // Calculate new endpoint on the edge of the circle

        this->p0 = p0.plus(14 * Vec(cos(angle), sin(angle)));
        this->p1 = p1.minus(14 * Vec(cos(angle), sin(angle)));
    }

    void drawBackground(const Widget::DrawArgs& args) override {}

    void drawLight(const Widget::DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, p0.x, p0.y);
        nvgLineTo(args.vg, p1.x, p1.y);
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

        indicatorPos = p0.plus(animTime * (p1.minus(p0)));
        alpha = std::sin(M_PI * animTime);
        TBase::step();
    }

private:
    Vec p0;
    Vec p1;
    float animTime = 0.0f;
    Vec indicatorPos = {p0.x, p0.y};

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