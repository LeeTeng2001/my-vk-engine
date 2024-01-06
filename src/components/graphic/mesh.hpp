#pragma once

#include <tiny_gltf.h>

#include "components/component.hpp"
#include "core/renderer/def.hpp"

namespace luna {

class MeshComponent: public Component {
public:
    explicit MeshComponent(const std::shared_ptr<Engine> &engine, int ownerId);
    ~MeshComponent() override;

    void postUpdate() override;

    // TODO: right now be like  this
    // can either load modal or procedurally generate one
    void loadModal(const std::string &path, const glm::vec3 &upAxis = glm::vec3{0, 1, 0});
    void generateSquarePlane(float sideLength, const glm::vec3 &color = glm::vec3{0, 0.1, 0.9});
    void generateSphere(float radius, int horizontalLine, int verticalLine, const glm::vec3 &color = glm::vec3{0.3, 0.8, 0.1});
    void uploadToGpu();

private:
    int createDefaultMat(const glm::vec3 &color);
    void generateTangentBitangent(int v0Idx, int v1Idx, int v2Idx);

    void loadObj(const std::string &path, const glm::vec3 &upAxis = glm::vec3{0, 1, 0});
    void loadGlb(const std::string &path, const glm::vec3 &upAxis = glm::vec3{0, 1, 0});
    void recurParseGlb(int nodeIdx, const tinygltf::Model& modal, const std::vector<int> &gpuMatId, ModelDataPartition &partition, const glm::mat4 &parentTransform);

    // Group model data based on material group
    ModelDataCpu _modelData;
    std::shared_ptr<ModalState> _modelState;
};

}