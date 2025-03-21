#include <thread>

#include "physic.hpp"

namespace luna {

// TODO: refactor those constant into config
constexpr int PhyTempAllocSize = 10 * 1024 * 1024;
constexpr int MaxBodies = 65536;
constexpr int MaxBodiesPairs = 65536;
constexpr int MaxContactConstrain = 10240;
constexpr int NumBodiesMutexes = 0;

bool PhysicSystem::initialise() {
    // TODO: customisable physic configuration from initialisation arg
    // setup example:
    // https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

    // JPH::Trace = TraceFunc // implement trace callback
    _joltAlloc =
        std::make_unique<JPH::TempAllocatorImpl>(PhyTempAllocSize);  // TODO: customise alloc impl
    _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1);  // TODO: customise job impl

    // factory and types
    JPH::Factory::sInstance = new JPH::Factory;
    JPH::RegisterTypes();

    _joltPhysicSystem.Init(MaxBodies, NumBodiesMutexes, MaxBodiesPairs, MaxContactConstrain,
                           _bpLayer, _objVsBroadPhaseLayer, _objLayerPairFilter);

    _joltPhysicSystem.SetBodyActivationListener(&_bodyListener);
    _joltPhysicSystem.SetContactListener(&_contactListener);

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This
    // improves collision detection performance (it's pointless here because we only have 2 bodies).
    // You should definitely not call this every frame or when e.g. streaming in a new level section
    // as it is an expensive operation. Instead insert all new objects in batches instead of 1 at a
    // time to keep the broad phase efficient. _joltPhysicSystem.OptimizeBroadPhase();

    return true;
}

void PhysicSystem::shutdown() {
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicSystem::update(float deltaTime) {
    _accumUpdateS += deltaTime;

    int totalCollisionSteps = 0;
    while (_fixedUpdateStepS < _accumUpdateS) {
        _accumUpdateS -= _fixedUpdateStepS;
        totalCollisionSteps++;
    }
    if (totalCollisionSteps == 0) {
        return;
    }

    if (totalCollisionSteps > 3) {
        auto l = SLog::get();
        l->warn(fmt::format("collision step drifting: {:d}", totalCollisionSteps));
    }

    // update
    _joltPhysicSystem.Update(_fixedUpdateStepS, totalCollisionSteps, _joltAlloc.get(),
                             _jobSystem.get());
}

}  // namespace luna