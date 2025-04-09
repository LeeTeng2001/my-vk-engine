#include "utils/common.hpp"
#include "lua.hpp"
#include "core/physic/physic.hpp"
#include "core/renderer/renderer.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "actors/object/point_light.hpp"
#include "actors/object/static.hpp"
#include "actors/object/empty.hpp"
#include "components/component.hpp"
#include "components/graphic/mesh.hpp"
#include "components/physic/rigidbody.hpp"
#include "components/anim/tween.hpp"

constexpr const char *SCRIPT_MODULE_PATH = "assets.scene.demo";

namespace luna {
bool ScriptingSystem::initialise(const std::shared_ptr<Engine> &engine) {
    _engine = engine;
    _globState.open_libraries();

    // https://edw.is/using-lua-with-cpp/
    // Register namespace functions
    // If object lifetime is managed by CPP, provide a Newxxx function
    // otherwise, register usertype with constructors

    registerGlm();
    registerLuna();

    return true;
}

void ScriptingSystem::registerGlm() {
    sol::table glmNs = _globState["glm"].get_or_create<sol::table>();
    glmNs.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(float, float, float)>(), "x",
                                  &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);
    glmNs.new_usertype<glm::quat>(
        "quat", sol::constructors<glm::quat(float, float, float, float)>(), "x", &glm::quat::x, "y",
        &glm::quat::y, "z", &glm::quat::z, "w", &glm::quat::w);
    glmNs.set_function("angleAxis", [](float angle, const glm::vec3 &axis) {
        return glm::angleAxis(angle, axis);
    });
    glmNs.set_function("radians", glm::radians<float>);
}

void ScriptingSystem::registerLuna() {
    // luna namespace
    sol::table lunaNs = _globState["luna"].get_or_create<sol::table>();
    lunaNs.new_usertype<LuaLog>("Log", "debug", &LuaLog::debug, "info", &LuaLog::info, "warn",
                                &LuaLog::warn, "error", &LuaLog::error);

    // Creation functions because lifetime is managed in CPP side
    lunaNs.set_function("GetEngine", [this]() { return _engine; });

    // graphics functions
    lunaNs.set_function("SetGraphicBgColor", [this](const glm::vec3 &color) {
        _engine->getRenderer()->setClearColor(color);
    });
    lunaNs.set_function("SetGraphicSunlight", [this](const glm::vec3 &dir, const glm::vec3 &color) {
        _engine->getRenderer()->setDirLight(glm::normalize(dir), color);
    });

    // base actor
    auto luaActor = lunaNs.new_usertype<Actor>("Actor");
    luaActor["setLocalPosition"] = &Actor::setLocalPosition;
    luaActor["getLocalPosition"] = &Actor::getLocalPosition;
    luaActor["setRotation"] = &Actor::setRotation;
    luaActor["setScale"] = &Actor::setScale;
    luaActor["getId"] = &Actor::getId;

    // inherit actor
    lunaNs.new_usertype<EmptyActor>("EmptyActor", sol::base_classes, sol::bases<Actor>());
    lunaNs.set_function("NewEmptyActor", [this]() {
        auto a = std::make_shared<EmptyActor>();
        _engine->addActor(a);
        return a;
    });
    lunaNs.new_usertype<CameraActor>("CameraActor", sol::base_classes, sol::bases<Actor>());
    lunaNs.set_function("NewCameraActor", [this]() {
        auto a = std::make_shared<CameraActor>();
        _engine->addActor(a);
        return a;
    });
    lunaNs.new_usertype<StaticActor>("StaticActor", sol::base_classes, sol::bases<Actor>());
    lunaNs.set_function("NewStaticActor", [this](const std::string &modelPath) {
        auto a = std::make_shared<StaticActor>(modelPath);
        _engine->addActor(a);
        return a;
    });
    lunaNs.new_usertype<PointLightActor>("PointLightActor", sol::base_classes, sol::bases<Actor>());
    lunaNs.set_function("NewPointLightActor", [this](const glm::vec3 &color, float radius) {
        auto a = std::make_shared<PointLightActor>(color, 0.3, radius);
        _engine->addActor(a);
        return a;
    });

    // base components
    lunaNs.new_usertype<Component>("Component");

    // TODO: use overload to reduce setup code
    // https://sol2.readthedocs.io/en/latest/api/overload.html inherit components
    lunaNs.new_usertype<MeshComponent>(
        "MeshComponent", sol::base_classes, sol::bases<Component>(), "generateSquarePlane",
        &MeshComponent::generateSquarePlane, "generateSphere", &MeshComponent::generateSphere,
        "loadModal", &MeshComponent::loadModal, "uploadToGpu", &MeshComponent::uploadToGpu);
    lunaNs.set_function("NewMeshComponent", [this](int actorId) {
        auto c = std::make_shared<MeshComponent>(_engine, actorId);
        _engine->getActor(actorId)->addComponent(c);
        return c;
    });

    lunaNs.new_usertype<RigidBodyComponent>(
        "RigidBodyComponent", sol::base_classes, sol::bases<Component>(), "setIsStatic",
        &RigidBodyComponent::setIsStatic, "setBounciness", &RigidBodyComponent::setBounciness,
        "createBox", &RigidBodyComponent::createBox, "createSphere",
        &RigidBodyComponent::createSphere, "setLinearVelocity",
        &RigidBodyComponent::setLinearVelocity);
    lunaNs.set_function("NewRigidBodyComponent", [this](int actorId) {
        auto c = std::make_shared<RigidBodyComponent>(_engine, actorId);
        _engine->getActor(actorId)->addComponent(c);
        return c;
    });

    lunaNs.new_usertype<TweenComponent>(
        "TweenComponent", sol::base_classes, sol::bases<Component>(), "addTranslateOffset",
        &TweenComponent::addTranslateOffset, "addRotationOffset",
        &TweenComponent::addRotationOffset, "setLoopType", &TweenComponent::setLoopType);
    lunaNs.set_function("NewTweenComponent", [this](int actorId) {
        auto c = std::make_shared<TweenComponent>(_engine, actorId);
        _engine->getActor(actorId)->addComponent(c);
        return c;
    });
}

void ScriptingSystem::shutdown() {
    _globState["luna"] = nullptr;
    _globState.collect_garbage();
    _globState = sol::state();  // destroy original
}

void ScriptingSystem::gc() {
    sol::table loaded = _globState["package"]["loaded"];
    for (auto &pair : loaded) {
        std::string key = pair.first.as<std::string>();
        if (key.find(SCRIPT_MODULE_PATH) == 0) {  // Check if the key starts with our script package
            // SLog::get()->info(fmt::format("removing cache lua package {:s}", key));
            loaded[key] = sol::nil;
        }
    }
    _globState.collect_garbage();
}

bool ScriptingSystem::execScriptFile(const std::string &path) {
    sol::load_result script = _globState.load_file(path);
    sol::protected_function_result res = script();
    if (!res.valid()) {
        auto l = SLog::get();
        sol::error err = res;
        l->error(fmt::format("failed to run lua script {:s}", err.what()));
        return false;
    }
    return true;
}

}  // namespace luna