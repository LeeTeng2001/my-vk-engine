#pragma once

#include "actors/actor.hpp"

class TweenComponent;
class MeshComponent;
class Engine;

class PointLightActor : public Actor {
public:
    explicit PointLightActor(glm::vec3 color = glm::vec3{1,1,1}, float ballSize = 0.3, float radius = 10) : Actor(), _color(color), _radius(radius) {
        setScale(ballSize);
    };

    void delayInit() override;
    void updateActor(float deltaTime) override;

private:
    shared_ptr<TweenComponent> _tweenComp;
    shared_ptr<MeshComponent> _meshComp;
    float _radius;
    glm::vec3 _color;
};
