#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

#include "core/physic/def.hpp"
#include "components/component.hpp"

class RigidBodyComponent : public Component {
public:
    explicit RigidBodyComponent(const shared_ptr<Engine> &engine, int ownerId);
    ~RigidBodyComponent() override;

    void postUpdate() override;

    // builder property
    void setIsStatic(bool isStatic) { _isStatic = isStatic; }
    void setBounciness(float bounciness) { _bounciness = bounciness; }
    void setRelativePos(glm::vec3 relPos) { _relPos = relPos; }

    // TODO: create body by fitting to mesh
    void createBox(const glm::vec3 &boxHalfExtentDim);
    void createSphere(float radius);

    // modify body properties
    void setLinearVelocity(const glm::vec3& velo);

private:
    void createShape(const JPH::ShapeRefC& bodyShapeRef);

    // utils
    [[nodiscard]] JPH::EMotionType getMotionType() const { return _isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic; }
    [[nodiscard]] JPH::ObjectLayer getMotionLayer() const { return _isStatic ? PhyObjLayers::NON_MOVING : PhyObjLayers::MOVING; }
    [[nodiscard]] JPH::EActivation getInitialActivation() const { return _isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate; }

    bool _isStatic = true;

    // property
    glm::vec3 _relPos{};
    float _bounciness = 0;  // [0,1]

    JPH::BodyID _bodyId;
};
