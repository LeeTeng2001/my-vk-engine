# executable name
set(EXE_NAME vulkan_engine)

# Supress warning in vma
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")

# Add source to this project's executable.
add_executable(${EXE_NAME}
        main.cpp
        engine/engine.cpp engine/engine.hpp
        engine/custom_type.hpp
        engine/creation_helper.cpp engine/creation_helper.hpp
        )

target_include_directories(${EXE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${EXE_NAME}
        ${Vulkan_LIBRARIES}   # Vulkan
        sdl2  # Window manager
        vk-bootstrap::vk-bootstrap  # Bootstrap vulkan
        vma  # Memory allocator by AMD
        glm  # Math lib
        imgui
        stb_image
        )

add_dependencies(${EXE_NAME} Shaders)

