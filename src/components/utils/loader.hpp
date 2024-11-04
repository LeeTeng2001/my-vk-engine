#pragma once

#include "components/component.hpp"

// Loader component is responsible for loading various assets
// and generate the corresponding world actors and components

namespace luna {

class LoaderComponent : public Component {
    public:
        explicit LoaderComponent(const std::shared_ptr<Engine> &engine, int ownerId);
        ~LoaderComponent() override;

        void loadModal(const std::string &path, const glm::vec3 &upAxis = glm::vec3{0, 1, 0});

    private:
        std::string _modelPath;
};

}  // namespace luna