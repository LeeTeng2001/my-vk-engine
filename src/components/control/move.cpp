#include <utility>

#include "move.hpp"
#include "actors/actor.hpp"

MoveComponent::MoveComponent(weak_ptr<Actor> owner, int updateOrder) : Component(std::move(owner), updateOrder) {
}

void MoveComponent::update(float deltaTime) {
    if (!getEnabled()) {
        return;
    }

    // TODO: Fix rotation
    if (glm::abs(_horizontalAngularSpeed) > 0.01 || glm::abs(_vertAngularSpeed) > 0.01) {
        // calculate the offset at base position first, then add back to original rotation
        glm::quat offset(1.0, 0.0, 0.0, 0.0);
        float horAngle = _horizontalAngularSpeed * deltaTime;
        float vertAngle = _vertAngularSpeed * deltaTime;
        // quaternion rotation order is L to R, unlike matrixes transformation order
        offset *= glm::angleAxis(glm::radians(vertAngle), glm::vec3(1.f, 0.f, 0.f));
        offset *= glm::angleAxis(glm::radians(horAngle), glm::vec3(0.f, 1.f, 0.f));
        offset *= getOwner()->getRotation();
        getOwner()->setRotation(offset);
    }
    if (glm::abs(_forwardSpeed) > 0.01 || glm::abs(_strafeSpeed) > 0.01) {
        glm::vec3 pos = getOwner()->getPosition();
        pos += getOwner()->getForward() * _forwardSpeed * deltaTime;
        pos += getOwner()->getRight() * _strafeSpeed * deltaTime;
        getOwner()->setPosition(pos);
    }
}
