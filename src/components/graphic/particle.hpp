#pragma once

#include "components/component.hpp"
#include "core/renderer/def.hpp"

namespace luna {

class ParticleComponent : public Component {
    public:
        explicit ParticleComponent(const std::shared_ptr<Engine> &engine, int ownerId);
        // ~ParticleComponent() override;

        // void postUpdate() override;

        // void generateRandomParticle(float sideLength,
        //                             const glm::vec3 &color = glm::vec3{0, 0.1, 0.9});

    private:
        // Group model data based on material group
        ModelDataCpu _modelData;
        std::shared_ptr<ModalState> _modelState;
};

}  // namespace luna