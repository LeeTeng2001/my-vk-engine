# executable name
set(EXE_NAME vulkan_engine)

# Add source to this project's executable.
add_executable(${EXE_NAME}
        main.cpp
        actors/actor.hpp
        actors/actor.cpp
        core/engine.hpp
        core/engine.cpp
        core/custom_type.hpp
        core/creation_helper.hpp
        core/input_system.hpp
        core/input_system.cpp
        core/camera.hpp
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
add_executable(CameraUnitTest tests/cameraTest.cpp)
target_include_directories(CameraUnitTest PUBLIC engine)
target_link_libraries(CameraUnitTest gtest gtest_main glm SDL3::SDL3)

