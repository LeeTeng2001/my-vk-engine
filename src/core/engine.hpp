#pragma once

#include "utils/common.hpp"

class Actor;
class Renderer;
class RenderConfig;
class InputSystem;
class CameraActor;
class StaticActor;

class Engine {
public:
    Engine() = default;
    virtual ~Engine();

    bool initialize(shared_ptr<Engine> &self);
    void run();

    bool prepareScene();
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

private:
    weak_ptr<Engine> _self;
    GameState _gameState = EGameplay;
    uint64_t _tickCountMs = 0;

    // System
    shared_ptr<Renderer> _renderer = nullptr;
    shared_ptr<InputSystem> _inputSystem = nullptr;

    // game specific member
    // TODO: refactor to game/scene class
    vector<shared_ptr<Actor>> _actorList;
    shared_ptr<CameraActor> _camActor = nullptr;
    shared_ptr<StaticActor> _staticActor = nullptr;
};
