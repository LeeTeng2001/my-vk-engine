#include "utils/common.hpp"
#include "utils/algo.hpp"
#include "lua.hpp"
#include "core/physic/physic.hpp"
#include "actors/actor.hpp"
#include "actors/player/camera.hpp"
#include "actors/object/static.hpp"
#include "actors/object/empty.hpp"
#include "components/component.hpp"
#include "components/graphic/mesh.hpp"
#include "components/physic/rigidbody.hpp"
#include "components/anim/tween.hpp"

#include <libplatform/libplatform.h>

namespace luna {
bool ScriptingSystem::initialise(const std::shared_ptr<Engine> &engine) {
    _engine = engine;

    // https://edw.is/using-lua-with-cpp/
    // Register namespace functions
    // If object lifetime is managed by CPP, provide a Newxxx function
    // otherwise, register usertype with constructors

    // init js runtime
    _v8platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(_v8platform.get());
    v8::V8::Initialize();

    // global isolate
    v8::Isolate::CreateParams create_params;
    _v8alloc = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    create_params.array_buffer_allocator = _v8alloc;
    _isolate = v8::Isolate::New(create_params);

    registerGlm();
    registerLuna();

    return true;
}

// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
    return *value ? *value : "<string conversion failed>";
}

void Print(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto l = SLog::get();
    bool first = true;
    for (int i = 0; i < info.Length(); i++) {
        v8::HandleScope handle_scope(info.GetIsolate());
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(info.GetIsolate(), info[i]);
        const char *cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}

void ScriptingSystem::registerGlm() {
    auto l = SLog::get();
    l->info("create glm");
    v8::HandleScope handleScope(_isolate);
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(_isolate);
    global->Set(_isolate, "print", v8::FunctionTemplate::New(_isolate, Print));

    // sol::table glmNs = _globState["glm"].get_or_create<sol::table>();
    // glmNs.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(float, float, float)>(),
    // "x",
    //                               &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);
    // glmNs.new_usertype<glm::quat>(
    //     "quat", sol::constructors<glm::quat(float, float, float, float)>(), "x", &glm::quat::x,
    //     "y", &glm::quat::y, "z", &glm::quat::z, "w", &glm::quat::w);
    // glmNs.set_function("angleAxis", [](float angle, const glm::vec3 &axis) {
    //     return glm::angleAxis(angle, axis);
    // });
    // glmNs.set_function("radians", glm::radians<float>);

    // return context
    v8::Local<v8::Context> context = v8::Context::New(_isolate, nullptr, global);
    _globCtx = v8::Global<v8::Context>(_isolate, context);
}

void ScriptingSystem::registerLuna() {
    // // luna namespace
    // sol::table lunaNs = _globState["luna"].get_or_create<sol::table>();
    // lunaNs.new_usertype<LuaLog>("Log", "debug", &LuaLog::debug, "info", &LuaLog::info, "warn",
    //                             &LuaLog::warn, "error", &LuaLog::error);

    // // Creation functions because lifetime is managed in CPP side
    // lunaNs.set_function("GetLog", []() { return SLog::get(); });
    // lunaNs.set_function("GetEngine", [this]() { return _engine; });

    // // base actor
    // auto luaActor = lunaNs.new_usertype<Actor>("Actor");
    // luaActor["setLocalPosition"] = &Actor::setLocalPosition;
    // luaActor["getLocalPosition"] = &Actor::getLocalPosition;
    // luaActor["setRotation"] = &Actor::setRotation;
    // luaActor["setScale"] = &Actor::setScale;
    // luaActor["getId"] = &Actor::getId;

    // // inherit actor
    // lunaNs.new_usertype<EmptyActor>("EmptyActor", sol::base_classes, sol::bases<Actor>());
    // lunaNs.set_function("NewEmptyActor", [this]() {
    //     auto a = std::make_shared<EmptyActor>();
    //     _engine->addActor(a);
    //     return a;
    // });
    // lunaNs.new_usertype<CameraActor>("CameraActor", sol::base_classes, sol::bases<Actor>());
    // lunaNs.set_function("NewCameraActor", [this]() {
    //     auto a = std::make_shared<CameraActor>();
    //     _engine->addActor(a);
    //     return a;
    // });
    // lunaNs.new_usertype<StaticActor>("StaticActor", sol::base_classes, sol::bases<Actor>());
    // lunaNs.set_function("NewStaticActor", [this](const std::string &modelPath) {
    //     auto a = std::make_shared<StaticActor>(modelPath);
    //     _engine->addActor(a);
    //     return a;
    // });

    // // base components
    // lunaNs.new_usertype<Component>("Component");

    // // TODO: use overload to reduce setup code
    // // https://sol2.readthedocs.io/en/latest/api/overload.html inherit components
    // lunaNs.new_usertype<MeshComponent>(
    //     "MeshComponent", sol::base_classes, sol::bases<Component>(), "generateSquarePlane",
    //     &MeshComponent::generateSquarePlane, "generateSphere", &MeshComponent::generateSphere,
    //     "loadModal", &MeshComponent::loadModal, "uploadToGpu", &MeshComponent::uploadToGpu);
    // lunaNs.set_function("NewMeshComponent", [this](int actorId) {
    //     auto c = std::make_shared<MeshComponent>(_engine, actorId);
    //     _engine->getActor(actorId)->addComponent(c);
    //     return c;
    // });

    // lunaNs.new_usertype<RigidBodyComponent>(
    //     "RigidBodyComponent", sol::base_classes, sol::bases<Component>(), "setIsStatic",
    //     &RigidBodyComponent::setIsStatic, "setBounciness", &RigidBodyComponent::setBounciness,
    //     "createBox", &RigidBodyComponent::createBox, "createSphere",
    //     &RigidBodyComponent::createSphere, "setLinearVelocity",
    //     &RigidBodyComponent::setLinearVelocity);
    // lunaNs.set_function("NewRigidBodyComponent", [this](int actorId) {
    //     auto c = std::make_shared<RigidBodyComponent>(_engine, actorId);
    //     _engine->getActor(actorId)->addComponent(c);
    //     return c;
    // });

    // lunaNs.new_usertype<TweenComponent>(
    //     "TweenComponent", sol::base_classes, sol::bases<Component>(), "addTranslateOffset",
    //     &TweenComponent::addTranslateOffset, "addRotationOffset",
    //     &TweenComponent::addRotationOffset, "setLoopType", &TweenComponent::setLoopType);
    // lunaNs.set_function("NewTweenComponent", [this](int actorId) {
    //     auto c = std::make_shared<TweenComponent>(_engine, actorId);
    //     _engine->getActor(actorId)->addComponent(c);
    //     return c;
    // });
}

void ScriptingSystem::shutdown() {
    _isolate->Dispose();
    delete _v8alloc;
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
}

bool ScriptingSystem::execScriptFile(const std::string &path) {
    auto l = SLog::get();
    // load file
    std::vector<char> scriptContent = HelperAlgo::readFile(path);

    // execute in context
    {
        v8::HandleScope handleScope(_isolate);
        v8::Context::Scope contextScope(_globCtx.Get(_isolate));
        v8::Local<v8::String> source =
            v8::String::NewFromUtf8(_isolate, scriptContent.data(), v8::NewStringType::kNormal,
                                    scriptContent.size())
                .ToLocalChecked();
        v8::Local<v8::Context> context(_isolate->GetCurrentContext());
        v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
        v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
        v8::String::Utf8Value utf8(_isolate, result);
        l->info(fmt::format("{:s} result {:s}", path, *utf8));
    }

    return true;
}

}  // namespace luna