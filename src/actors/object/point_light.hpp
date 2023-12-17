#pragma once

#include "actors/actor.hpp"

class TweenComponent;
class Engine;

class PointLightActor : public Actor {
public:
    explicit PointLightActor(glm::vec3 color = glm::vec3{1,1,1}, float radius = 10) : Actor(), _color(color), _radius(radius) {};

    void delayInit() override;
    void updateActor(float deltaTime) override;

private:
    shared_ptr<TweenComponent> _tweenComp;
    float _radius;
    glm::vec3 _color;
};
