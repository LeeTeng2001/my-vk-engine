#pragma once

#include "actors/actor.hpp"

namespace luna {

class MoveComponent;
class Engine;

class CameraActor : public Actor {
public:
    explicit CameraActor(float nearDepth = 0.1, float farDepth = 100, float fovYinAngle = 60) : Actor(),
                    _nearDepth(nearDepth), _farDepth(farDepth), _fovYInAngle(fovYinAngle) {};

    void delayInit() override;
    void updateActor(float deltaTime) override;
    void actorInput(const struct InputState &state) override;
    std::string displayName() override { return "CameraActor"; }

    // utils
    glm::mat4 getCamViewTransform();
    glm::mat4 getPerspectiveTransformMatrix();
    glm::mat4 getOrthographicTransformMatrix();

private:
    // cam config
    float _nearDepth;
    float _farDepth;
    float _fovYInAngle;

    // rotation, look to z
    float _pitchAngle = 0; // head updown [-85, 85], ++ up
    float _yawAngle = 0; // head rightleft [0, 360], ++ to the right, clockwise

    std::shared_ptr<MoveComponent> _moveComp;
};

}
