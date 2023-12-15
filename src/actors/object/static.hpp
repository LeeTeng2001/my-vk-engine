#pragma once

#include "actors/actor.hpp"

class MeshComponent;
class Engine;

class StaticActor : public Actor {
public:
    explicit StaticActor(const std::string &modelPath, std::string diffusePath = "") :
                    Actor(), _modelPath(modelPath), _diffuseTexPath(diffusePath) {};

    void delayInit() override;

private:
    std::string _modelPath;
    std::string _diffuseTexPath;
    shared_ptr<MeshComponent> _meshComp;
};
