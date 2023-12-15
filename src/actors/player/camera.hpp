#pragma once

#include "actors/actor.hpp"

class MoveComponent;
class Engine;

class CameraActor : public Actor {
public:
    explicit CameraActor() : Actor() {};

    void delayInit() override;
    void updateActor(float deltaTime) override;
    void actorInput(const struct InputState &state) override;

    // utils
    glm::mat4 getCamViewTransform();
    glm::mat4 getPerspectiveTransformMatrix();

private:
    shared_ptr<MoveComponent> _moveComp;
};
