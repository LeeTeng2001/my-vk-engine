#pragma once

#include "components/component.hpp"

namespace luna {

class AnimationComponent : public Component {
    public:
        explicit AnimationComponent(const std::shared_ptr<Engine> &engine, int ownerId);
        ~AnimationComponent() override;
};

// TODO: finish it
}  // namespace luna
