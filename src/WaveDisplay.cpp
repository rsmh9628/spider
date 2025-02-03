#include "WaveDisplay.hpp"
#include "Colour.hpp"

namespace ph {

WaveDisplay::WaveDisplay() {
    this->box.size = mm2px(Vec(21.602, 10.993));
}

void WaveDisplay::draw(const DrawArgs& args) {
    //if (!wavetable) {
    //    return;
    //}
}

void WaveDisplay::drawLayer(const DrawArgs& args, int layer) {
    if (layer != 1)
        return;

    nvgScissor(args.vg, RECT_ARGS(args.clipBox));

    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFillPaint(args.vg, nvgLinearGradient(args.vg, 0, 0, box.size.x, 32, nvgRGB(7, 7, 9), nvgRGB(23, 12, 94)));
    nvgFill(args.vg);

    auto innerBox = box;
    innerBox.pos.x += 4;
    innerBox.pos.y += 4;
    innerBox.size.x -= 8;
    innerBox.size.y -= 8;

    nvgBeginPath(args.vg);
    nvgRect(args.vg, 2, 2, box.size.x - 4, box.size.y - 4);
    nvgStrokeColor(args.vg, OPERATOR_COLOURS[op]);
    nvgStrokeWidth(args.vg, 0.5f);
    nvgStroke(args.vg);

    nvgBeginPath(args.vg);
    // TODO: too much detail for now
    size_t numPoints = 64;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (size_t i = 0; i <= numPoints; ++i) {
        float t = float(i) / numPoints;
        float sineValue = amplitude * sin(2.0f * M_PI * frequency * t);

        Vec point;
        point.x = t;
        point.y = 0.5f - sineValue;

        point = innerBox.size * point;

        if (i == 0) {
            nvgMoveTo(args.vg, point.x + 4, point.y + 4);
        } else {
            nvgLineTo(args.vg, point.x + 4, point.y + 4);
        }
    }

    // nvgMiterLimit(args.vg, 2.f);
    nvgStrokeWidth(args.vg, 1.f);
    nvgLineCap(args.vg, NVG_ROUND);
    nvgStrokeColor(args.vg, OPERATOR_COLOURS[op]);
    nvgStroke(args.vg);

    // nvgFillPaint(args.vg,
    //              nvgLinearGradient(args.vg, 0, 0, 0, innerBox.size.y / 2, OPERATOR_COLOURS[op], nvgRGBA(0, 0, 0,
    //              0)));
    // nvgFill(args.vg);
}

} // namespace ph