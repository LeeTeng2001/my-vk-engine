#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include "actors/actor.hpp"
#include "core/physic/physic.hpp"
#include "core/physic/def.hpp"
#include "core/engine.hpp"
#include "rigidbody.hpp"


// https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

RigidBodyComponent::RigidBodyComponent(const shared_ptr<Engine> &engine, int ownerId, bool isStatic, int updateOrder) :
                                        Component(engine, ownerId, updateOrder), _isStatic(isStatic) {
}

RigidBodyComponent::~RigidBodyComponent() {
    if (_simBody != nullptr) {
        JPH::BodyInterface& bodyInf = getOwner()->getEngine()->getPhysicSystem()->getBodyInf();
        bodyInf.RemoveBody(_simBody->GetID());
        bodyInf.DestroyBody(_simBody->GetID());
    }
}

void RigidBodyComponent::createBox(const glm::vec3 &boxHalfExtentDim, const glm::vec3 &relPos) {
    JPH::BodyInterface& bodyInf = getOwner()->getEngine()->getPhysicSystem()->getBodyInf();

    // shape setting
    JPH::BoxShapeSettings shapeSetting(JPH::Vec3(boxHalfExtentDim.x, boxHalfExtentDim.y, boxHalfExtentDim.z));
    JPH::ShapeSettings::ShapeResult boxShapeRes = shapeSetting.Create();
    JPH::ShapeRefC boxShapeRef = boxShapeRes.Get();
    if (boxShapeRes.HasError()) {
        auto l = SLog::get();
        l->error(fmt::format("Create box shape error: {:s}", boxShapeRes.GetError()));
        return;
    }

    // Create the actual rigid body
    glm::vec3 finalPos = relPos + getOwner()->getWorldPosition();
    JPH::BodyCreationSettings boxSetting(boxShapeRef, JPH::RVec3(finalPos.x, finalPos.y, finalPos.z),
                                         JPH::Quat::sIdentity(), getJPHMotionType(), PhyObjLayers::NON_MOVING);
    JPH::Body* rigidBody = bodyInf.CreateBody(boxSetting); // Note that if we run out of bodies this can return nullptr
    if (rigidBody == nullptr) {
        auto l = SLog::get();
        l->error("Create box rigidbody return nullptr");
        return;
    }
    _simBody = std::unique_ptr<JPH::Body>(rigidBody);

    // add to physic world
    if (_isStatic) {
        bodyInf.AddBody(_simBody->GetID(), JPH::EActivation::DontActivate);
    } else {
        bodyInf.AddBody(_simBody->GetID(), JPH::EActivation::Activate);
    }
}

