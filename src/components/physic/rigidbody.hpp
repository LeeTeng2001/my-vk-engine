#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

#include "components/component.hpp"

class RigidBodyComponent : public Component {
public:
    explicit RigidBodyComponent(const shared_ptr<Engine> &engine, int ownerId, bool isStatic, int updateOrder);
    ~RigidBodyComponent() override;

    // TODO: create body by fitting to mesh
    void createBox(const glm::vec3 &boxHalfExtentDim, const glm::vec3 &relPos = glm::vec3{0, 0, 0});
    void createSphere(const glm::vec3 &radius, const glm::vec3 &relPos = glm::vec3{0, 0, 0});

private:
    // utils
    [[nodiscard]] JPH::EMotionType getJPHMotionType() const { return _isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic; }

    bool _isStatic = true;

    unique_ptr<JPH::Body> _simBody;
};
