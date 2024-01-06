#pragma once

#include "actors/actor.hpp"

namespace luna {

class Engine;

class EmptyActor : public Actor {
public:
    explicit EmptyActor() : Actor() {};

    void delayInit() override;
    std::string displayName() override { return "EmptyActor"; }
};

}
