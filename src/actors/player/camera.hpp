#pragma once

#include "actors/actor.hpp"

class MoveComponent;
class Engine;

class CameraActor : public Actor {
public:
    explicit CameraActor(float nearDepth = 0.1, float farDepth = 100, float fovYinAngle = 60) : Actor(),
                    _nearDepth(nearDepth), _farDepth(farDepth), _fovYInAngle(fovYinAngle) {};

    void delayInit() override;
    void updateActor(float deltaTime) override;
    void actorInput(const struct InputState &state) override;

    // utils
    glm::mat4 getCamViewTransform();
    glm::mat4 getPerspectiveTransformMatrix();

private:
    // cam config
    float _nearDepth;
    float _farDepth;
    float _fovYInAngle;

    shared_ptr<MoveComponent> _moveComp;
};
