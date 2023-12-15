#include <SDL.h>

#include "engine.hpp"
#include "actors/actor.hpp"
#include "renderer/renderer.hpp"
#include "input_system.hpp"

bool Engine::initialize(shared_ptr<Engine> &self) {
    _self = self;
    auto l = SLog::get();

    _renderer = make_shared<Renderer>();
    if (!_renderer->initialise()) { // custom config
        l->error("failed to initialise renderer");
        return false;
    }
    _inputSystem = make_shared<InputSystem>();
    if (!_inputSystem->initialise()) {
        l->error("failed to initialise input system");
        return false;
    }

    // TODO: upload modal

    return true;
}

void Engine::run() {
    while (_gameState != EQuit) {
        processInput();
        updateGame();
        drawOutput();
    }
}

Engine::~Engine() {

}

void Engine::processInput() {
    _inputSystem->prepareForUpdate();

    // POLL for keyboard event
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                _gameState = EQuit;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                _inputSystem->processEvent(event);
                break;
        }
    }

    _inputSystem->update();
    const InputState& state = _inputSystem->getState();

    // propagate input to actors / ui
    if (_gameState == EGameplay) {
//        mUpdatingActors = true;
//        for (auto actor: mActors) {
//            actor->ProcessInput(state);
//        }
//        mUpdatingActors = false;
    }
}

void Engine::updateGame() {
    // Renderer
    _renderer->newFrame();
    _renderer->beginRecordCmd();

}

void Engine::drawOutput() {
    _renderer->endRecordCmd();
    _renderer->draw();
}

void Engine::addActor(const shared_ptr<Actor>& actor) {
    actor->setParent(actor, _self.lock());
    _actorList.push_back(actor);
}

