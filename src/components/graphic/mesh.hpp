#pragma once

#include "components/component.hpp"
#include "core/renderer/def.hpp"

class MeshComponent: public Component {
public:
    explicit MeshComponent(weak_ptr<Actor> owner, int updateOrder = 10);
    ~MeshComponent() override;

    void onUpdateWorldTransform() override;

    // TODO: right now be like  this
    // can either load modal or procedurally generate one
    void loadModal(const std::string &path);
    void generatedSquarePlane(float sideLength);

    void loadDiffuseTexture(const std::string &path);
    void loadNormalTexture(const std::string &path);
    void uploadToGpu();

private:
    ModelData _modelData;
    shared_ptr<ModalState> _modelState;
};

