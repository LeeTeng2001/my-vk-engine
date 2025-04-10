#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include "def.hpp"
#include "utils/common.hpp"

// jolt physic has the same coordinate as our world space

namespace luna {

class PhysicSystem {
    public:
        bool initialise();
        void shutdown();

        void update(float deltaTime);

        void static preInit() {
            JPH::RegisterDefaultAllocator();
        }  // WINDOWS behaviour, this should be setup before making a new physic system

        // exposed methods
        JPH::BodyInterface& getBodyInf() {
            return _joltPhysicSystem.GetBodyInterface();
        }  // locking, thread safe

    private:
        // update related
        const float _fixedUpdateStepS = 1.0f / 60.0f;
        float _accumUpdateS = 0;

        // for temp alloc, rn fix it at 10MB
        std::unique_ptr<JPH::TempAllocatorImpl> _joltAlloc;
        std::unique_ptr<JPH::JobSystemThreadPool> _jobSystem;

        // layers and broadcast implementation
        BPLayerInterfaceImpl _bpLayer{};
        ObjectVsBroadPhaseLayerFilterImpl _objVsBroadPhaseLayer{};
        ObjectLayerPairFilterImpl _objLayerPairFilter{};
        MyBodyActivationListener _bodyListener;
        MyContactListener _contactListener;

        JPH::PhysicsSystem _joltPhysicSystem{};
};

}  // namespace luna