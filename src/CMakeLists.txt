# executable name
set(EXE_NAME vulkan_engine)

# Add source to this project's executable.
add_executable(${EXE_NAME}
        main.cpp

        actors/actor.tpp
        actors/actor.hpp
        actors/actor.cpp
        actors/player/camera.cpp
        actors/player/camera.hpp
        actors/object/static.cpp
        actors/object/static.hpp
        actors/object/point_light.cpp
        actors/object/point_light.hpp
        actors/object/empty.cpp
        actors/object/empty.hpp

        components/component.hpp
        components/component.cpp
#        components/anim/animation.cpp
#        components/anim/animation.hpp
        components/anim/tween.hpp
        components/anim/tween.cpp
        components/control/move.hpp
        components/control/move.cpp
        components/graphic/mesh.cpp
        components/graphic/mesh.hpp
        components/physic/rigidbody.cpp
        components/physic/rigidbody.hpp
        components/utils/loader.cpp
        components/utils/loader.hpp

        core/engine.hpp
        core/engine.cpp
        core/input/input_system.hpp
        core/input/input_system.cpp
        core/scripting/lua.cpp
        core/scripting/lua.hpp
        core/physic/def.hpp
        core/physic/physic.cpp
        core/physic/physic.hpp
        core/renderer/renderer.hpp
        core/renderer/renderer.cpp
        core/renderer/creation_helper.hpp
        core/renderer/def.hpp
        core/renderer/builder.hpp
        core/renderer/builder.cpp

        utils/lib_impl.cpp
        utils/common.hpp
        utils/log.hpp
        utils/log.cpp
        utils/algo.hpp
)

target_include_directories(${EXE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${EXE_NAME}
        ${Vulkan_LIBRARIES}   # Vulkan for graphic
        SDL3::SDL3  # Window manager & a lot more
        vk-bootstrap::vk-bootstrap  # Bootstrap vulkan
        GPUOpen::VulkanMemoryAllocator  # Memory allocator by AMD
        glm  # Math lib
        tinyobjloader # model loader
        imgui
        stb_image
        tinygltf
        spdlog::spdlog
        Jolt # physic
        # Lua scripting
        lua
        sol2::sol2
)

# For MSVC profiler
if (MSVC AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "debug mode + MSVC, adding profile flag")
    set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS "/PROFILE")
endif()

add_dependencies(${EXE_NAME} Shaders)

# Unit tests ----------------------------------------------------------------------
add_executable(MathUnitTest tests/math_test.cpp utils/common.hpp)
target_include_directories(MathUnitTest PUBLIC engine)
target_link_libraries(MathUnitTest gtest gtest_main glm SDL3::SDL3 spdlog::spdlog)

