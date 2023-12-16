#include <utility>

#include "move.hpp"
#include "actors/actor.hpp"

MoveComponent::MoveComponent(weak_ptr<Actor> owner, int updateOrder) : Component(std::move(owner), updateOrder) {
}

void MoveComponent::update(float deltaTime) {
    // TODO: Fix rotation
    if (glm::abs(_horizontalAngularSpeed) > 0.01 || glm::abs(_vertAngularSpeed) > 0.01) {
        // calculate the offset at base position first, then add back to original rotation
        glm::quat offset = glm::angleAxis(0.0f, glm::vec3(1.f, 0.f, 0.f));
        float horAngle = _horizontalAngularSpeed * deltaTime;
        float vertAngle = _vertAngularSpeed * deltaTime;
        offset = glm::angleAxis(glm::radians(vertAngle), glm::vec3(1.f, 0.f, 0.f)) * offset;
        offset = glm::angleAxis(glm::radians(horAngle), glm::vec3(0.f, 1.f, 0.f)) * offset;
        offset = _owner->getRotation() * offset;
        _owner->setRotation(offset);
    }
    if (glm::abs(_forwardSpeed) > 0.01 || glm::abs(_strafeSpeed) > 0.01) {
        glm::vec3 pos = _owner->getPosition();
        pos += _owner->getForward() * _forwardSpeed * deltaTime;
        pos += _owner->getRight() * _strafeSpeed * deltaTime;
        _owner->setPosition(pos);
    }
}
