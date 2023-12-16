#include <SDL.h>
#include <backends/imgui_impl_sdl3.h>

#include "core/engine.hpp"
#include "core/input_system.hpp"
#include "renderer/renderer.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "actors/object/static.hpp"
#include "components/anim/tween.hpp"

bool Engine::initialize(shared_ptr<Engine> &self) {
    _self = self;
    auto l = SLog::get();

    _renderer = make_shared<Renderer>();
    if (!_renderer->initialise()) { // TODO: custom config
        l->error("failed to initialise renderer");
        return false;
    }
    _inputSystem = make_shared<InputSystem>();
    if (!_inputSystem->initialise()) {
        l->error("failed to initialise input system");
        return false;
    }

    if (!prepareScene()) {
        l->error("failed to prepare scene");
        return false;
    }

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
        ImGui_ImplSDL3_ProcessEvent(&event);
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

    // propagate input to global / actors / ui
    if (_gameState == EGameplay) {
        handleGlobalInput(state);
        for (const auto& actor: _actorList) {
            actor->processInput(state);
        }
    }
}

void Engine::updateGame() {
    // Simple solution for frame limiting.
    // Wait until 16ms has elapsed since last frame
//    while (!SDL_TICKS_PASSED(SDL_GetTicks(), _tickCountMs + 16));

    // Get delta in second
    uint64_t curTicks = SDL_GetTicks();
    float deltaTime = static_cast<float>(curTicks - _tickCountMs) / 1000.0f;
    _tickCountMs = curTicks;

    // Clamp maximum delta to prevent huge delta time (ex, when stepping through debugger)
    deltaTime = std::min(deltaTime, 0.05f);

    // Only update in gameplay mode
    if (_gameState == EGameplay) {
        // Update all existing actors
//        mUpdatingActors = true;
        for (auto &actor: _actorList) {
            actor->update(deltaTime);
        }
//        mUpdatingActors = false;

//        // Check dead vector and remove
//        for (auto &actor: _actorList) {
//            if (actor->getState() == Actor::EDead) {
//            }
//        }
    }
}

void Engine::drawOutput() {
    // Renderer
    _renderer->newFrame();
    _renderer->beginRecordCmd();
    _renderer->drawAllModel();
    _renderer->endRecordCmd();
    _renderer->draw();
}

void Engine::addActor(const shared_ptr<Actor>& actor) {
    actor->delayInit(actor, _self.lock());
    _actorList.push_back(actor);
}

bool Engine::prepareScene() {
    _camActor = make_shared<CameraActor>();
    addActor(_camActor);
    auto s = make_shared<StaticActor>("assets/models/viking_room.obj", "assets/textures/viking_room.png");
    addActor(s);
    s = make_shared<StaticActor>("assets/models/cube.obj", "assets/textures/dice.png");
    addActor(s);
    auto tweenComp = make_shared<TweenComponent>(s);
    tweenComp->addTranslateOffset(3, glm::vec3{0,0,5});
    tweenComp->addTranslateOffset(3, glm::vec3{0,0,-5});
    s->addComponent(tweenComp);

    return true;
}

void Engine::handleGlobalInput(const InputState& key) {
    if (key.Keyboard.getKeyState(SDL_SCANCODE_ESCAPE) == EPressed) {
        auto l = SLog::get();
        l->info("detected exit key, exiting");
        _gameState = EQuit;
    }
}
