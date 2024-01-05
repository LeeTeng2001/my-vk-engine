#include "utils/common.hpp"
#include "lua.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "components/component.hpp"

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
    glmNs.new_usertype<glm::vec3>("vec3",
                                  sol::constructors<glm::vec3(float, float, float)>(),
                                  "x", &glm::vec3::x,
                                  "y", &glm::vec3::y,
                                  "z", &glm::vec3::z
    );
}

void ScriptingSystem::registerLuna() {
    // luna namespace
    sol::table lunaNs = _globState["luna"].get_or_create<sol::table>();
    lunaNs.new_usertype<LuaLog>("Log",
                                "debug", &LuaLog::debug,
                                "info", &LuaLog::info,
                                "warn", &LuaLog::warn,
                                "error", &LuaLog::error
    );

    // Creation functions because lifetime is managed in CPP side
    lunaNs.set_function("GetLog", [](){ return SLog::get(); });
    lunaNs.set_function("GetEngine", [this](){ return _engine; });
    lunaNs.set_function("NewCameraActor", [this](){
        auto a = std::make_shared<CameraActor>();
        _engine->addActor(a);
        return a;
    });

    // register actors methods
    auto luaActor = lunaNs.new_usertype<Actor>("Actor");
    luaActor["setLocalPosition"] = &Actor::setLocalPosition;
    luaActor["getLocalPosition"] = &Actor::getLocalPosition;
    lunaNs.new_usertype<CameraActor>("CameraActor", sol::base_classes, sol::bases<Actor>());

    // register components methods

}

void ScriptingSystem::shutdown() {
    _globState["luna"] = nullptr;
    _globState.collect_garbage();
    _globState = sol::state();  // destroy original
}

bool ScriptingSystem::execScriptFile(const std::string& path) {
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

}