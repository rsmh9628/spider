#pragma once

#include <rack.hpp>

namespace ph {

class FMOperator {
    using Args = rack::engine::Module::ProcessArgs;

public:
    enum ParamType {
        PARAM_RATIO,
        PARAM_DEPTH,
        PARAM_FEEDBACK,
        PARAM_LEVEL,
        PARAM_COUNT,
    };

    FMOperator(rack::Module* parent, unsigned char operatorID)
        : parent_(parent)
        , operatorID_(operatorID) {}

    void generate(const Args& args, float freq);
    void modulate(const Args& args, const FMOperator& modulator, float freq);
    float out() const { return out_; }

    float depth() const { return getParam(PARAM_DEPTH).getValue(); }
    float feedback() const { return getParam(PARAM_FEEDBACK).getValue(); }

private:
    rack::Param& getParam(ParamType type) const {
        return parent_->getParam(operatorID_ * 4 + type);
    }

    unsigned char operatorID_ = 0;

    float phase_ = 0.f;
    float out_ = 0.f;

    rack::Module* parent_;
};

} // namespace ph