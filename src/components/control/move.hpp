#pragma once

#include "components/component.hpp"

class MoveComponent : public Component {
public:
    explicit MoveComponent(const shared_ptr<Engine> &engine, int ownerId);

    void update(float deltaTime) override;

    void setHorAngularSpeed(float speed) { _horizontalAngularSpeed = speed; }
    void setVertAngularSpeed(float speed) { _vertAngularSpeed = speed; }
    void setForwardSpeed(float speed) { _forwardSpeed = speed; }
    void setStrafeSpeed(float speed) { _strafeSpeed = speed; }

private:
    // Controls rotation (angle/second)
    float _horizontalAngularSpeed = 0;
    float _vertAngularSpeed = 0;
    // Controls forward movement (units/second)
    float _forwardSpeed = 0;
    // Controls side movement (units/second)
    float _strafeSpeed = 0;
};
