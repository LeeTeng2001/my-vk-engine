#pragma once

#include <v8.h>

#include "core/engine.hpp"

namespace luna {
class ScriptingSystem {
    public:
        bool initialise(const std::shared_ptr<Engine>& engine);
        void shutdown();

        bool execScriptFile(const std::string& path);

    private:
        void registerGlm();
        void registerLuna();

        // global v8 state
        std::shared_ptr<Engine> _engine;
        std::unique_ptr<v8::Platform> _v8platform;
        v8::ArrayBuffer::Allocator* _v8alloc;
        v8::Isolate* _isolate;
        v8::Global<v8::Context> _globCtx;
};
}  // namespace luna