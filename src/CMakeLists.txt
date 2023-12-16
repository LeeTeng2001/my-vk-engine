# executable name
set(EXE_NAME vulkan_engine)

# Add source to this project's executable.
add_executable(${EXE_NAME}
        main.cpp

        actors/actor.hpp
        actors/actor.cpp
        actors/player/camera.cpp
        actors/player/camera.hpp
        actors/object/static.cpp
        actors/object/static.hpp

        components/component.hpp
        components/component.cpp
        components/anim/tween.hpp
        components/anim/tween.cpp
        components/control/move.hpp
        components/control/move.cpp
        components/graphic/mesh.cpp
        components/graphic/mesh.hpp

        core/camera.hpp
        core/engine.hpp
        core/engine.cpp
        core/input_system.hpp
        core/input_system.cpp

        renderer/renderer.hpp
        renderer/renderer.cpp
        renderer/creation_helper.hpp
        renderer/def.hpp
        renderer/builder.hpp
        renderer/builder.cpp

        utils/lib_impl.cpp
        utils/common.hpp
        utils/log.hpp
        utils/log.cpp
)

target_include_directories(${EXE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${EXE_NAME}
        ${Vulkan_LIBRARIES}   # Vulkan for graphic
        SDL3::SDL3  # Window manager & a lot more
        vk-bootstrap::vk-bootstrap  # Bootstrap vulkan
        vma  # Memory allocator by AMD
        glm  # Math lib
        tinyobjloader # model loader
        imgui
        stb_image
        tinygltf
        spdlog::spdlog
        )

add_dependencies(${EXE_NAME} Shaders)

# Unit tests ----------------------------------------------------------------------
add_executable(CameraUnitTest tests/cameraTest.cpp utils/common.hpp)
target_include_directories(CameraUnitTest PUBLIC engine)
target_link_libraries(CameraUnitTest gtest gtest_main glm SDL3::SDL3 spdlog::spdlog)

