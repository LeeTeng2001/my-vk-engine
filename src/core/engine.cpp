#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>

#include "core/engine.hpp"
#include "core/input/input_system.hpp"
#include "core/physic/physic.hpp"
#include "core/renderer/renderer.hpp"
#include "core/scripting/lua.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "actors/object/static.hpp"
#include "actors/object/empty.hpp"
#include "actors/object/point_light.hpp"
#include "components/anim/tween.hpp"
#include "components/graphic/mesh.hpp"
#include "components/physic/rigidbody.hpp"

namespace luna {
bool Engine::initialize(std::shared_ptr<Engine> &self) {
    _self = self;
    auto l = SLog::get();

    _renderer = std::make_shared<Renderer>();
    if (!_renderer->initialise()) {  // TODO: custom config
        l->error("failed to initialise renderer");
        return false;
    }
    _inputSystem = std::make_shared<InputSystem>();
    if (!_inputSystem->initialise()) {
        l->error("failed to initialise input system");
        return false;
    }
    PhysicSystem::preInit();
    _physicSystem = std::make_shared<PhysicSystem>();
    if (!_physicSystem->initialise()) {
        l->error("failed to initialise physic system");
        return false;
    }
    _scriptSystem = std::make_shared<ScriptingSystem>();
    if (!_scriptSystem->initialise(self)) {
        l->error("failed to initialise scripting system");
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
    if (_scriptSystem) _scriptSystem->shutdown();
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
            default:
                _inputSystem->processEvent(event);
                break;
        }
    }

    _inputSystem->update();
    const InputState &state = _inputSystem->getState();

    // propagate input to global / actors / ui
    handleGlobalInput(state);
    if (_gameState == EGameplay) {
        for (const auto &actorIter : _actorMap) {
            actorIter.second->processInput(state);
        }
    }
}

void Engine::updateGame() {
    // if we're reloading, destroy every resources and reload
    if (_gameState == EReload) {
        destroyScene();
        if (prepareScene()) {
            _gameState = EGameplay;
        } else {
            _gameState = EPaused;  // wait for valid script
            return;
        }
    }

    // Simple solution for frame limiting.
    // Wait until 16ms has elapsed since last frame
    Uint64 deadline = SDL_GetTicks() + 16;
    while (SDL_GetTicks() < deadline);

    // Get delta in second
    uint64_t curTicks = SDL_GetTicks();
    float deltaTime = static_cast<float>(curTicks - _tickCountMs) / 1000.0f;
    _tickCountMs = curTicks;
    _renderer->writeDebugUi(fmt::format("FPS:  {:d}", int(1.0f / deltaTime)));
    //    _renderer->writeDebugUi(fmt::format("Draw: {:d}us", _lastDrawFrameTimeUs));  // TODO:
    //    Benchmark each phase draw call time

    // Clamp maximum delta to prevent huge delta time (ex, when stepping through debugger)
    deltaTime = std::min(deltaTime, 0.05f);

    // Only update in gameplay mode
    if (_gameState == EGameplay) {
        // Update all existing actors
        for (auto &actorIter : _actorMap) {
            actorIter.second->update(deltaTime);
        }

        // update physic
        _physicSystem->update(deltaTime);

        // Check dead vector and remove
        for (auto actorIter = _actorMap.begin(); actorIter != _actorMap.end();) {
            if (actorIter->second->getState() == Actor::EDead) {
                actorIter = _actorMap.erase(actorIter);
            } else {
                ++actorIter;
            }
        }
    }
}

void Engine::drawOutput() {
    // TODO: too crude, should consider ui frame
    if (_gameState != EGameplay) return;
    // new frame
    _renderer->newFrame();
    _renderer->beginRecordCmd();
    _renderer->drawAllModel();
    drawDebugUi();
    _renderer->endRecordCmd();
    _renderer->draw();
}

void Engine::drawDebugUi() {
    if (ImGui::Begin("Engine##ActorHierarchy")) {
        for (const auto &actorIter : _actorMap) {
            // find root
            if (actorIter.second->getParentId() == -1) {
                drawDebugUiActorRecursive(actorIter.second);
            }
        }
        ImGui::End();
    }
}

void Engine::drawDebugUiActorRecursive(const std::shared_ptr<Actor> &actor) {
    ImGuiTreeNodeFlags initFlag = actor->getDebugUiExpand() ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (ImGui::TreeNodeEx(actor->debugDisplayName().c_str(), initFlag)) {
        actor->setDebugUiExpand(true);
        for (const auto &childId : actor->getChildrenIdList()) {
            auto childActor = _actorMap.find(childId);
            if (childActor == _actorMap.end()) {
                SLog::get()->error(
                    fmt::format("actor has child id {:d} but it is not found in engine!", childId));
                return;
            }
            drawDebugUiActorRecursive(childActor->second);
        }
        ImGui::TreePop();
    } else {
        actor->setDebugUiExpand(false);
    }
}

void Engine::addActor(const std::shared_ptr<Actor> &actor) {
    _actorMap.emplace(_actorIdInc, actor);
    actor->delayInit(_actorIdInc, _self.lock());
    _actorIdInc++;
}

bool Engine::prepareScene() {
    // rely on external lua script to setup scene
    // flexible!
    if (!_scriptSystem->execScriptFile("assets/scene/scene.lua")) {
        return false;
    }

    //    // viking room
    //    emptyActor = std::make_shared<EmptyActor>();
    //    addActor(emptyActor);
    //    meshComp = std::make_shared<MeshComponent>(emptyActor);
    //    meshComp->loadModal("assets/models/viking_room.obj", glm::vec3{0, 0, 1});
    //    meshComp->uploadToGpu();
    //    emptyActor->addComponent(meshComp);

    //    // moving cube with a parent
    //    emptyActor = std::make_shared<EmptyActor>();
    //    addActor(emptyActor);
    //    emptyActor->setLocalPosition(glm::vec3{-1, 0, 0});
    //    staticActor = std::make_shared<StaticActor>("assets/models/cube.obj");
    //    addActor(staticActor);
    //    staticActor->setParent(emptyActor->getId());
    //    tweenComp = std::make_shared<TweenComponent>(_self.lock(), staticActor->getId());
    //    tweenComp->addRotationOffset(3, -360, glm::vec3{0, 0, 1});
    //    staticActor->addComponent(tweenComp);

    //    // procedural floor plane
    //    emptyActor = std::make_shared<EmptyActor>();
    //    addActor(emptyActor);
    //    meshComp = std::make_shared<MeshComponent>(emptyActor);
    //    meshComp->generatedSquarePlane(2);
    //    meshComp->loadDiffuseTexture("assets/textures/Gravel_001_BaseColor.jpg");
    //    meshComp->loadNormalTexture("assets/textures/Gravel_001_Normal.jpg");
    //    meshComp->uploadToGpu();
    //    emptyActor->addComponent(meshComp);
    //    emptyActor->setLocalPosition(glm::vec3{0, 0, -1});
    //    emptyActor->setRotation(glm::angleAxis(glm::radians(-90.0f), glm::vec3{1, 0, 0}));

    // Light
    //    auto lightAct = std::make_shared<PointLightActor>(glm::vec3{1, 1, 1});
    //    addActor(lightAct);
    //    lightAct->setLocalPosition(glm::vec3{0, 5, 0});
    //    tweenComp = std::make_shared<TweenComponent>(lightAct);
    //    tweenComp->addTranslateOffset(3, glm::vec3{5, 0, 0}).addTranslateOffset(6, glm::vec3{-10,
    //    0, 0}).addTranslateOffset(3, glm::vec3{5, 0, 0}); lightAct->addComponent(tweenComp);

    // Dir light, right now set it like this
    // getRenderer()->setDirLight(glm::normalize(glm::vec3{1, -1, 0}), glm::vec3{1, 1, 1});

    return true;
}

void Engine::destroyScene() {
    auto l = SLog::get();
    _scriptSystem->gc();  // important to release unused reference
    l->info(fmt::format("destroying scene: actor count {:d}", _actorMap.size()));
    for (const auto &pair : _actorMap) {
        l->info(fmt::format("{:s} reference {:d}", pair.second->displayName(),
                            pair.second.use_count()));
    }
    _actorMap.clear();
}

void Engine::handleGlobalInput(const InputState &key) {
    if (key.Keyboard.getKeyState(SDL_SCANCODE_ESCAPE) == EPressed) {
        auto l = SLog::get();
        l->info("detected exit key, exiting");
        _gameState = EQuit;
    }
    if (key.Keyboard.getKeyState(SDL_SCANCODE_R) == EPressed) {
        auto l = SLog::get();
        l->info("detected reload key, rebuilding world from script");
        _gameState = EReload;
    }
}
}  // namespace luna