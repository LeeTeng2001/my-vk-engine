#pragma once

// https://stackoverflow.com/questions/48488460/how-to-safely-call-a-c-function-from-lua
#define SOL_SAFE_USERTYPE 1
#include <sol/sol.hpp>

#include "core/engine.hpp"

namespace luna {
class ScriptingSystem {
public:
    bool initialise(const std::shared_ptr<Engine> &engine);
    void shutdown();

    bool execScriptFile(const std::string& path);

private:
    sol::state _globState;
    std::shared_ptr<Engine> _engine;
};
}