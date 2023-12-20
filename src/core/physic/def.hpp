#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "utils/common.hpp"

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace PhyBroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
};
namespace PhyObjLayers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[PhyObjLayers::NON_MOVING] = PhyBroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[PhyObjLayers::MOVING] = PhyBroadPhaseLayers::MOVING;
    }
    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override {
        return PhyBroadPhaseLayers::NUM_LAYERS;
    }
    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < PhyObjLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }
    [[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)PhyBroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)PhyBroadPhaseLayers::MOVING:		return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[PhyObjLayers::NUM_LAYERS];
};

// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case PhyObjLayers::NON_MOVING:
                return inLayer2 == PhyBroadPhaseLayers::MOVING;
            case PhyObjLayers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// Class that determines if an object layer can collide with an object layer
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case PhyObjLayers::NON_MOVING:
                return inObject2 == PhyObjLayers::MOVING; // Non moving only collides with moving
            case PhyObjLayers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class MyBodyActivationListener : public JPH::BodyActivationListener {
public:
    void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        auto l = SLog::get();
        l->info(fmt::format("body get activated {:d}", inBodyID.GetIndex()));
    }
    void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        auto l = SLog::get();
        l->info(fmt::format("body went to sleep {:d}", inBodyID.GetIndex()));
    }
};

class MyContactListener : public JPH::ContactListener {
public:
    JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2,
                                             JPH::RVec3Arg inBaseOffset,
                                             const JPH::CollideShapeResult &inCollisionResult) override {
        auto l = SLog::get();
        l->debug("contact validate callback");
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
                        const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        auto l = SLog::get();
        l->debug("a contact was added");
    }

    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2,
                            const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        auto l = SLog::get();
        l->debug("a contact was persisted");
    }

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
        auto l = SLog::get();
        l->debug("a contact was removed");
    }
};