#pragma once

#include "actors/actor.hpp"

namespace luna {

class MeshComponent;
class Engine;

class StaticActor : public Actor {
public:
    explicit StaticActor(std::string modelPath = "") : Actor(), _modelPath(std::move(modelPath)) {};

    void delayInit() override;
    std::string displayName() override { return "StaticActor"; }

private:
    std::string _modelPath;
    std::shared_ptr<MeshComponent> _meshComp;
};

}
