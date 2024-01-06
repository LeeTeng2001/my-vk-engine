#include "actor.hpp"
#include "components/component.hpp"
#include "core/input/input_system.hpp"
#include "core/engine.hpp"
#include "glm/gtx/string_cast.hpp"

namespace luna {

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

void Actor::addComponent(const std::shared_ptr<Component>& component) {
    // lower order at front with multiset property
    _components.insert(component);
}

void Actor::removeComponent(const std::shared_ptr<Component>& component) {
    auto iter = std::find(_components.begin(), _components.end(), component);
    if (iter != _components.end()) {
        _components.erase(iter);
    }
}

void Actor::setParent(int parentId) {
    if (_parentId != -1) {
        // remove original parent reference
        auto p = _engine->getActor(_parentId);
        if (p == nullptr) {
            auto l = SLog::get();
            l->error(fmt::format("get engine actor returns null, id: {:d}", _parentId));
            return;
        } else {
            p->removeChild(getId());
        }
    }

    _parentId = parentId;
    auto p = _engine->getActor(_parentId);
    if (p == nullptr) {
        auto l = SLog::get();
        l->error(fmt::format("get engine actor returns null, id: {:d}", _parentId));
        return;
    } else {
        p->addChild(getId());
    }
}

void Actor::addChild(int childId) {
    _childrenIdList.insert(childId);
}

void Actor::removeChild(int childId) {
    _childrenIdList.erase(childId);
}

const glm::mat4 &Actor::getLocalTransform() {
    if (_recomputeLocalTransform) {
        _recomputeLocalTransform = false;
        // Scale, then rotate, then translate
        glm::mat4 scaleMat{};
        scaleMat[0][0] = _scale;
        scaleMat[1][1] = _scale;
        scaleMat[2][2] = _scale;
        scaleMat[3][3] = 1;
        _localTransform = glm::translate(glm::identity<glm::mat4>(), _position) * glm::mat4_cast(_rotation) * scaleMat;
    }
    return _localTransform;
}

glm::mat4 Actor::getWorldTransform() {
    // TODO: optimise into cache
    glm::mat4 worldTransform = getLocalTransform();
    int recParentId = _parentId;
    while (recParentId != -1) {
        auto p = _engine->getActor(recParentId);
        if (p == nullptr) {
            auto l = SLog::get();
            l->error(fmt::format("get engine actor returns null, id: {:d}", recParentId));
            break;
        } else {
            worldTransform = p->getLocalTransform() * worldTransform;
            recParentId = p->getParentId();
        }
    }
    return worldTransform;
}

glm::vec3 Actor::getWorldPosition() const {
    // TODO: optimise into cache
    glm::vec3 finalPos = getLocalPosition();
    int recParentId = _parentId;
    while (recParentId != -1) {
        auto p = _engine->getActor(recParentId);
        if (p == nullptr) {
            auto l = SLog::get();
            l->error(fmt::format("get engine actor returns null, id: {:d}", recParentId));
            break;
        } else {
            finalPos += p->getLocalPosition();
            recParentId = p->getParentId();
        }
    }
    return finalPos;
}

void Actor::setWorldPosition(const glm::vec3 &pos) {
    glm::vec3 relPos = pos;
    int recParentId = _parentId;
    while (recParentId != -1) {
        auto p = _engine->getActor(recParentId);
        if (p == nullptr) {
            auto l = SLog::get();
            l->error(fmt::format("get engine actor returns null, id: {:d}", recParentId));
            break;
        } else {
            relPos -= p->getLocalPosition();
            recParentId = p->getParentId();
        }
    }
    _position = relPos;
    _recomputeLocalTransform = true;
}

}