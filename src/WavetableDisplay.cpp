#include "WavetableDisplay.hpp"

namespace ph {

WavetableDisplay::WavetableDisplay() {
    this->box.size = mm2px(Vec(21.602, 10.993));
}

void WavetableDisplay::draw(const DrawArgs& args) {
    if (!wavetable) {
        return;
    }
}

void WavetableDisplay::drawLayer(const DrawArgs& args, int layer) {
    if (!wavetable)
        return;
    if (layer != 1)
        return;

    float wavePos = *this->wavePos;

    float pos = wavePos - std::trunc(wavePos);
    size_t wavePos0 = std::trunc(wavePos);
    size_t wavePos1 = wavePos + 1;

    nvgBeginPath(args.vg);
    // TODO: too much detail for now
    size_t increment = wavetable->wavelength / 64 + 1;

    for (size_t i = 0; i <= wavetable->wavelength; i += increment) {
        float index0 = wavetable->sampleAt(wavePos0, i % wavetable->wavelength);
        float index1 = wavetable->sampleAt(wavePos1, i % wavetable->wavelength);

        float index = crossfade(index0, index1, pos);

        Vec point;
        point.x = (float(i) / wavetable->wavelength);
        point.y = 0.5f - 0.5f * index;

        point = box.size * point;

        if (i == 0) {
            nvgMoveTo(args.vg, point.x, point.y);
        } else {
            nvgLineTo(args.vg, point.x, point.y);
        }
    }

    // nvgMiterLimit(args.vg, 2.f);
    nvgStrokeWidth(args.vg, 1.5f);
    nvgLineCap(args.vg, NVG_ROUND);
    nvgStrokeColor(args.vg, nvgRGB(0xff, 0x00, 0xff));
    nvgStroke(args.vg);
}

void WavetableDisplay::onButton(const ButtonEvent& e) {
    // Right click to open context menu
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && (e.mods & RACK_MOD_MASK) == 0) {
        createContextMenu();
        e.consume(this);
    }
}

void WavetableDisplay::createContextMenu() {
    Menu* menu = createMenu();

    menu->addChild(createMenuItem("Init wavetable", "", [this]() {
        if (wavetable) {
            wavetable->init();
        }
    }));

    menu->addChild(createMenuItem("Open wavetable", "", [this]() {
        if (wavetable) {
            DEFER({ wavetable->openDialog(); });
        }
    }));

    int maxSamples = wavetable->sampleCount();

    const int sizeMin = 5; // 2^5 == 32 samples
    const int sizeMax = std::ceil(log2(maxSamples));

    std::vector<std::string> sizeLabels;

    for (int i = sizeMin; i <= sizeMax; i++) {
        sizeLabels.push_back(string::f("%d", 1 << i)); // 1 << i == 2^i
    }

    menu->addChild(createIndexSubmenuItem(
        "Set wavelength", sizeLabels,
        [=]() {
            return log2(wavetable->wavelength) - sizeMin;
        },
        [=](int i) {
            wavetable->wavelength = 1 << (i + sizeMin); // 2^i
        }));
}

} // namespace ph