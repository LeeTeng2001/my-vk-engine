#include "utils/common.hpp"

#include "lua.hpp"

bool luna::ScriptingSystem::initialise(const std::shared_ptr<Engine> &engine) {
    _engine = engine;

    // https://edw.is/using-lua-with-cpp/
    _globState.open_libraries();

    // Register a bunch of global scripting type for script
    auto glmNs = _globState["glm"].get_or_create<sol::table>();
//    glmNs.new_usertype<glm::vec4>()

    // luna
    auto lunaNs = _globState["luna"].get_or_create<sol::table>();
    lunaNs.new_usertype<LuaLog>("log",
                                "debug", &LuaLog::debug,
                                "info", &LuaLog::info,
                                "warn", &LuaLog::warn,
                                "error", &LuaLog::error
                                );

    return true;
}

void luna::ScriptingSystem::shutdown() {
    _globState = sol::state();  // destroy original
}

bool luna::ScriptingSystem::execScriptFile(const std::string& path) {
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
