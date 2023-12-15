#pragma once

#include "utils/common.hpp"

class Actor;
class Renderer;
class InputSystem;

class Engine {
public:
    Engine() = default;
    virtual ~Engine();

    bool initialize(shared_ptr<Engine> &self);
    void run();

    void processInput();
    void updateGame();
    void drawOutput();

    enum GameState {
        EGameplay,
        EPaused,
        EQuit
    };

    // Create or delete actors
    void addActor(const shared_ptr<Actor>& actor);
//    void removeActor(shared_ptr<Actor> actor);

    // Core Getter accessed by subsystem
    shared_ptr<Renderer> getRenderer() { return _renderer; }
    shared_ptr<InputSystem> getInputSystem() { return _inputSystem; }

    // utils
    shared_ptr<Engine> getSelf();

private:
    weak_ptr<Engine> _self;
    shared_ptr<Renderer> _renderer = nullptr;
    shared_ptr<InputSystem> _inputSystem = nullptr;
    GameState _gameState = EGameplay;
    uint64_t _tickCountMs = 0;

    // game specific member
    vector<shared_ptr<Actor>> _actorList;
};
