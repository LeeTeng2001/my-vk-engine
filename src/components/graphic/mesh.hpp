#pragma once

#include "components/component.hpp"
#include "renderer/def.hpp"

class MeshComponent: public Component {
public:
    explicit MeshComponent(weak_ptr<Actor> owner, int updateOrder = 10);
    ~MeshComponent() override;

    void onUpdateWorldTransform() override;

    // TODO: right now like this
    void loadModal(const std::string &path);
    void loadDiffuseTexture(const std::string &path);
    void uploadToGpu();

private:
    ModelData _modelData;
    shared_ptr<ModalState> _modelState;
};

