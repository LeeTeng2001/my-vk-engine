#include "actor.hpp"
#include "components/component.hpp"
#include "core/input/input_system.hpp"
#include "glm/gtx/string_cast.hpp"

Actor::~Actor() {
    // Need to delete components, because ~Component calls RemoveComponent, need a different style loop
    _components.clear();
}

void Actor::update(float deltaTime) {
    if (_state == EActive) {
        updateComponents(deltaTime);
        updateActor(deltaTime);
        for (const auto &comp: _components) {
            comp->postUpdate();
        }
    }
}

void Actor::updateComponents(float deltaTime) {
    for (const auto& comp: _components) {
        comp->update(deltaTime);
    }
}

void Actor::processInput(const struct InputState &keyState) {
    // process input for components, then actor specific
    if (_state == EActive) {
        for (const auto& comp: _components) {
            comp->processInput(keyState);
        }
        actorInput(keyState);
    }
}

void Actor::addComponent(const shared_ptr<Component>& component) {
    // lower order at front with multiset property
    _components.insert(component);
}

void Actor::removeComponent(const shared_ptr<Component>& component) {
    auto iter = std::find(_components.begin(), _components.end(), component);
    if (iter != _components.end()) {
        _components.erase(iter);
    }
}

void Actor::setParent(const shared_ptr<Actor>& parent) {
    if (!_parentPtr.expired()) {
        // remove original parent reference
        _parentPtr.lock()->removeChild(getSelf().lock());
    }
    _parentPtr = parent;
    parent->addChild(getSelf().lock());

    // TODO: if update priority is set, update along
}

void Actor::addChild(const shared_ptr<Actor>& child) {
    _childrenPtrList.insert(child);
}

void Actor::removeChild(const shared_ptr<Actor>& child) {
    auto iter = std::find(_childrenPtrList.begin(), _childrenPtrList.end(), child);
    if (iter != _childrenPtrList.end()) {
        _childrenPtrList.erase(iter);
    }
}

const glm::mat4 &Actor::getLocalTransform() {
    if (_recomputeLocalTransform) {
        _recomputeLocalTransform = false;
        // Scale, then rotate, then translate
        _localTransform = glm::translate(glm::identity<glm::mat4>(), _position) * glm::mat4_cast(_rotation) * glm::identity<glm::mat4>() * _scale;
    }
    return _localTransform;
}

glm::mat4 Actor::getWorldTransform() {
    // TODO: optimise into cache
    glm::mat4 worldTransform = getLocalTransform();
    weak_ptr<Actor> parentPtr = _parentPtr;
    while (!parentPtr.expired()) {
        auto p = parentPtr.lock();
        worldTransform = p->getLocalTransform() * worldTransform;
        parentPtr = p->getParent();
    }
    return worldTransform;
}
