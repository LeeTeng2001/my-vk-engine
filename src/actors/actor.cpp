#include "actor.hpp"
#include "components/component.hpp"
#include "core/input_system.hpp"
//#include "../Game.hpp"

Actor::~Actor() {
//    mGame->RemoveActor(this);
    // Need to delete components, because ~Component calls RemoveComponent, need a different style loop
    while (!_components.empty()) {
        delete *_components.end();
    }
}

void Actor::update(float deltaTime) {
    if (_state == EActive) {
        computeWorldTransform();
        updateComponents(deltaTime);
        updateActor(deltaTime);
        computeWorldTransform();
    }
}

void Actor::updateComponents(float deltaTime) {
    for (auto comp: _components) {
        comp->update(deltaTime);
    }
}

void Actor::processInput(const struct InputState &keyState) {
    // process input for components, then actor specific
    if (_state == EActive) {
        for (auto comp: _components) {
            comp->processInput(keyState);
        }
        actorInput(keyState);
    }
}

void Actor::addComponent(struct Component *component) {
    // lower order at front with multiset property
    _components.insert(component);
}

void Actor::removeComponent(Component *component) {
    auto iter = std::find(_components.begin(), _components.end(), component);
    if (iter != _components.end()) {
        _components.erase(iter);
    }
}

void Actor::computeWorldTransform() {
    if (_recomputeWorldTransform) {
        _recomputeWorldTransform = false;

//        // Scale, then rotate, then translate
//        mWorldTransform = Matrix4::CreateScale(mScale);
//        mWorldTransform *= Matrix4::CreateFromQuaternion(mRotation);
//        mWorldTransform *= Matrix4::CreateTranslation(mPosition);
//
//        // Inform components world transform updated
//        for (auto comp: mComponents) {
//            comp->OnUpdateWorldTransform();
//        }
    }
}

//void Actor::RotateToNewForward(const Vector3 &forward) {
//    // Figure out difference between original (unit x) and new
//    float dot = Vector3::Dot(Vector3::UnitX, forward);
//    float angle = Math::Acos(dot);
//    if (dot > 0.9999f) {
//        // Facing down X
//        SetRotation(Quaternion::Identity);
//    }
//    else if (dot < -0.9999f) {
//        // Facing down -X
//        SetRotation(Quaternion(Vector3::UnitZ, Math::Pi));
//    } else {
//        // Rotate about axis from cross product
//        Vector3 axis = Vector3::Cross(Vector3::UnitX, forward);
//        axis.Normalize();
//        SetRotation(Quaternion(axis, angle));
//    }
//}