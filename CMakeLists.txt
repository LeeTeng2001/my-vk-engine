cmake_minimum_required(VERSION 3.22)
project(my_vk_engine)
set(CMAKE_CXX_STANDARD 20)

# debug preprocessor symbol
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("building in debug mode")
    add_compile_definitions(DEBUG)
endif()

# If you're on windows: please use MSVC instead of MingW to avoid a lot of pain
find_package(Vulkan REQUIRED)

# Necessary config for Jolt
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(libs)
add_subdirectory(src)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp"
)

file(GLOB_RECURSE GLSL_UTILS_FILES
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.glsl"
)

# Check for glsl lang compiler 
if((NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE))
    message(FATAL_ERROR "glslang validation does not exist in path")
endif()

## iterate each shader and compile
foreach (GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    ##execute glslang command to compile that specific shader
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL} ${GLSL_UTILS_FILES})  # TODO: optimise dependencies
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
