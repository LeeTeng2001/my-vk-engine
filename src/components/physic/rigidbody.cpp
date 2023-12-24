#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "actors/actor.hpp"
#include "core/physic/physic.hpp"
#include "core/physic/def.hpp"
#include "core/engine.hpp"
#include "rigidbody.hpp"


// https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

RigidBodyComponent::RigidBodyComponent(const shared_ptr<Engine> &engine, int ownerId) : Component(engine, ownerId) {
}

RigidBodyComponent::~RigidBodyComponent() {
    if (!_bodyId.IsInvalid()) {
        JPH::BodyInterface& bodyInf = getEngine()->getPhysicSystem()->getBodyInf();
        bodyInf.RemoveBody(_bodyId);
        bodyInf.DestroyBody(_bodyId);
    }
}

void RigidBodyComponent::postUpdate() {
    if (_bodyId.IsInvalid()) return;
    JPH::BodyInterface& bodyInf = getEngine()->getPhysicSystem()->getBodyInf();
    if (!bodyInf.IsActive(_bodyId)) return;
    JPH::RVec3 position = bodyInf.GetCenterOfMassPosition(_bodyId);
    getOwner()->setWorldPosition(glm::vec3{position[0], position[1], position[2]});
}

void RigidBodyComponent::createBox(const glm::vec3 &boxHalfExtentDim) {
    // shape setting (verbose)
    JPH::BoxShapeSettings shapeSetting(JPH::Vec3(boxHalfExtentDim.x, boxHalfExtentDim.y, boxHalfExtentDim.z));
    JPH::ShapeSettings::ShapeResult boxShapeRes = shapeSetting.Create();
    if (boxShapeRes.HasError()) {
        auto l = SLog::get();
        l->error(fmt::format("Create box shape error: {:s}", boxShapeRes.GetError()));
        return;
    }
    JPH::ShapeRefC boxShapeRef = boxShapeRes.Get();
    createShape(boxShapeRef);
}

void RigidBodyComponent::createSphere(float radius) {
    // shape setting (concise)
    createShape(new JPH::SphereShape(radius));
}

void RigidBodyComponent::createShape(const JPH::ShapeRefC& bodyShapeRef) {
    JPH::BodyInterface& bodyInf = getEngine()->getPhysicSystem()->getBodyInf();

    // Create the actual rigid body
    glm::vec3 finalPos = _relPos + getOwner()->getWorldPosition();
    JPH::BodyCreationSettings bodySetting(bodyShapeRef, JPH::RVec3(finalPos.x, finalPos.y, finalPos.z),
                                          JPH::Quat::sIdentity(), getMotionType(), getMotionLayer());
    bodySetting.mRestitution = _bounciness;
    _bodyId = bodyInf.CreateAndAddBody(bodySetting, getInitialActivation());
    if (_bodyId.IsInvalid()) {
        auto l = SLog::get();
        l->error("Create rigidbody returns invalid body id, possibly out of MAX collisions body");
    }
}


void RigidBodyComponent::setLinearVelocity(const glm::vec3& velo) {
    JPH::BodyInterface& bodyInf = getEngine()->getPhysicSystem()->getBodyInf();
    bodyInf.SetLinearVelocity(_bodyId, JPH::Vec3(velo[0], velo[1], velo[2]));
}
