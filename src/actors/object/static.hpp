#pragma once

#include "actors/actor.hpp"

class MeshComponent;
class Engine;

class StaticActor : public Actor {
public:
    explicit StaticActor(std::string modelPath = "") : Actor(), _modelPath(std::move(modelPath)) {};

    void delayInit() override;

private:
    std::string _modelPath;
    shared_ptr<MeshComponent> _meshComp;
};
