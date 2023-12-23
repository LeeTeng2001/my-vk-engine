#include <SDL.h>
#include <backends/imgui_impl_sdl3.h>

#include "core/engine.hpp"
#include "core/input/input_system.hpp"
#include "core/physic/physic.hpp"
#include "core/renderer/renderer.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "actors/object/static.hpp"
#include "actors/object/empty.hpp"
#include "actors/object/point_light.hpp"
#include "components/anim/tween.hpp"
#include "components/graphic/mesh.hpp"

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
    PhysicSystem p;
    _physicSystem = make_shared<PhysicSystem>();
    if (!_physicSystem->initialise()) {
        l->error("failed to initialise physic system");
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
    if (_inputSystem) _inputSystem->shutdown();
    if (_renderer) _renderer->shutdown();
    if (_physicSystem) _physicSystem->shutdown();
};

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
        for (const auto& actorIter: _actorMap) {
            actorIter.second->processInput(state);
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
    _renderer->writeDebugUi(fmt::format("FPS: {:d}", int(1.0f / deltaTime)));

    // Clamp maximum delta to prevent huge delta time (ex, when stepping through debugger)
    deltaTime = std::min(deltaTime, 0.05f);

    // Only update in gameplay mode
    if (_gameState == EGameplay) {
        // Update all existing actors
        for (auto &actorIter: _actorMap) {
            actorIter.second->update(deltaTime);
        }

        // update physic
        _physicSystem->update(deltaTime);

        // Check dead vector and remove
        for (auto actorIter = _actorMap.begin(); actorIter != _actorMap.end();) {
            if (actorIter->second->getState() == Actor::EDead) {
                actorIter = _actorMap.erase(actorIter);
            } else { ++actorIter; }
        }
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
    _actorMap.emplace(_actorIdInc, actor);
    actor->delayInit(_actorIdInc, _self.lock());
    _actorIdInc++;
}

bool Engine::prepareScene() {
    _camActor = make_shared<CameraActor>();
    addActor(_camActor);
    _camActor->setLocalPosition(glm::vec3{0, 0.5, 3});

    shared_ptr<Actor> staticActor;
    shared_ptr<EmptyActor> emptyActor;
    shared_ptr<TweenComponent> tweenComp;
    shared_ptr<MeshComponent> meshComp;

//    staticActor = make_shared<StaticActor>("assets/models/miniature_night_city.obj");
//    addActor(staticActor);

//    // viking room
//    emptyActor = make_shared<EmptyActor>();
//    addActor(emptyActor);
//    meshComp = make_shared<MeshComponent>(emptyActor);
//    meshComp->loadModal("assets/models/viking_room.obj", glm::vec3{0, 0, 1});
//    meshComp->uploadToGpu();
//    emptyActor->addComponent(meshComp);

//    // moving cube with a parent
//    emptyActor = make_shared<EmptyActor>();
//    addActor(emptyActor);
//    emptyActor->setLocalPosition(glm::vec3{-1, 0, 0});
//    staticActor = make_shared<StaticActor>("assets/models/cube.obj");
//    addActor(staticActor);
//    staticActor->setParent(emptyActor->getId());
//    tweenComp = make_shared<TweenComponent>(_self.lock(), staticActor->getId());
//    tweenComp->addRotationOffset(3, -360, glm::vec3{0, 0, 1});
//    staticActor->addComponent(tweenComp);

//    // procedural floor plane
//    emptyActor = make_shared<EmptyActor>();
//    addActor(emptyActor);
//    meshComp = make_shared<MeshComponent>(emptyActor);
//    meshComp->generatedSquarePlane(2);
//    meshComp->loadDiffuseTexture("assets/textures/Gravel_001_BaseColor.jpg");
//    meshComp->loadNormalTexture("assets/textures/Gravel_001_Normal.jpg");
//    meshComp->uploadToGpu();
//    emptyActor->addComponent(meshComp);
//    emptyActor->setLocalPosition(glm::vec3{0, 0, -1});
//    emptyActor->setRotation(glm::angleAxis(glm::radians(-90.0f), glm::vec3{1, 0, 0}));

    // Light
//    auto lightAct = make_shared<PointLightActor>(glm::vec3{1, 1, 1});
//    addActor(lightAct);
//    lightAct->setLocalPosition(glm::vec3{0, 5, 0});
//    tweenComp = make_shared<TweenComponent>(lightAct);
//    tweenComp->addTranslateOffset(3, glm::vec3{5, 0, 0}).addTranslateOffset(6, glm::vec3{-10, 0, 0}).addTranslateOffset(3, glm::vec3{5, 0, 0});
//    lightAct->addComponent(tweenComp);

    // Dir light, right now set it like this
    getRenderer()->setDirLight(glm::normalize(glm::vec3{1, -1, 0}), glm::vec3{1, 1, 1});

    return true;
}

void Engine::handleGlobalInput(const InputState& key) {
    if (key.Keyboard.getKeyState(SDL_SCANCODE_ESCAPE) == EPressed) {
        auto l = SLog::get();
        l->info("detected exit key, exiting");
        _gameState = EQuit;
    }
}
