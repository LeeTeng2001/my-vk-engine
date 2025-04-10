#include <utility>

#include "move.hpp"
#include "actors/actor.hpp"

namespace luna {

MoveComponent::MoveComponent(const std::shared_ptr<Engine> &engine, int ownerId)
    : Component(engine, ownerId) {}

void MoveComponent::update(float deltaTime) {
    if (!getEnabled()) {
        return;
    }

    // TODO: Fix rotation
    if (glm::abs(_horizontalAngularSpeed) > 0.01 || glm::abs(_vertAngularSpeed) > 0.01) {
        //        // calculate the offset at base position first, then add back to original rotation
        //        glm::quat offset(1.0, 0.0, 0.0, 0.0);
        //        float horAngle = _horizontalAngularSpeed * deltaTime;
        //        float vertAngle = _vertAngularSpeed * deltaTime;
        //        // TODO: FUCK fix this
        //        // quaternion rotation order is L to R, unlike matrixes transformation order
        //        offset *= glm::angleAxis(glm::radians(vertAngle), getOwner()->getRight());
        //        offset *= glm::angleAxis(glm::radians(horAngle), glm::vec3(0.f, 1.f, 0.f));
        //        offset = getOwner()->getRotation() * offset;
        //        getOwner()->setRotation(offset);
        // rotate in euler space
        float horAngle = _horizontalAngularSpeed * deltaTime;
        float vertAngle = _vertAngularSpeed * deltaTime;
        glm::quat res = getOwner()->getRotation();
        res = glm::rotate(res, -glm::radians(horAngle), getOwner()->getUp());
        res = glm::rotate(res, glm::radians(vertAngle), getOwner()->getRight());
        getOwner()->setRotation(res);
    }
    if (glm::abs(_forwardSpeed) > 0.01 || glm::abs(_strafeSpeed) > 0.01) {
        glm::vec3 pos = getOwner()->getLocalPosition();
        pos += getOwner()->getForward() * _forwardSpeed * deltaTime;
        pos += getOwner()->getRight() * _strafeSpeed * deltaTime;
        getOwner()->setLocalPosition(pos);
    }
}

}  // namespace luna