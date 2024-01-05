#pragma once

#include <unordered_map>

#include "core/input/input_system.hpp"
#include "utils/common.hpp"

namespace luna {
class Renderer;
class RenderConfig;
class InputSystem;
class PhysicSystem;
class ScriptingSystem;
class Actor;
class CameraActor;
class StaticActor;

class Engine {
public:
    Engine() = default;
    virtual ~Engine();

    bool initialize(std::shared_ptr<Engine> &self);
    void run();

    bool prepareScene();
    void processInput();
    void updateGame();
    void drawOutput();
    void handleGlobalInput(const InputState& key);

    enum GameState {
        EGameplay,
        EPaused,
        EQuit
    };

    // Create or delete actors
    void addActor(const std::shared_ptr<Actor>& actor);
    std::shared_ptr<Actor> getActor(int actorId) { auto iter = _actorMap.find(actorId); return iter == _actorMap.end() ? nullptr : iter->second;};

    // Core Getter accessed by subsystem
    std::shared_ptr<Renderer> getRenderer() { return _renderer; }
    std::shared_ptr<InputSystem> getInputSystem() { return _inputSystem; }
    std::shared_ptr<PhysicSystem> getPhysicSystem() { return _physicSystem; }

private:
    std::weak_ptr<Engine> _self;
    GameState _gameState = EGameplay;
    uint64_t _tickCountMs = 0;
    int _actorIdInc = 0;

    // System
    std::shared_ptr<Renderer> _renderer = nullptr;
    std::shared_ptr<InputSystem> _inputSystem = nullptr;
    std::shared_ptr<PhysicSystem> _physicSystem = nullptr;
    std::shared_ptr<ScriptingSystem> _scriptSystem = nullptr;

    // game specific member
    // TODO: refactor to game/scene class
    std::unordered_map<int, std::shared_ptr<Actor>> _actorMap;
    std::shared_ptr<CameraActor> _camActor = nullptr;
};
}