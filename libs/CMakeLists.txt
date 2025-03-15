# CPM package manager
file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=c0fc82149e00c43a21febe7b2ca57b2ffea2b8e88ab867022c21d6b81937eb50
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# You should have Vulkan installed on your machine
find_package(Vulkan REQUIRED)

# add easy to include 3rd party packages
CPMAddPackage(
        GITHUB_REPOSITORY jrouwe/JoltPhysics
        GIT_TAG v5.2.0
        SOURCE_SUBDIR "Build"
)
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.8")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.2.1")
CPMAddPackage(
        GITHUB_REPOSITORY syoyo/tinygltf
        GIT_TAG v2.9.5
        OPTIONS "TINYGLTF_INSTALL OFF" "TINYGLTF_HEADER_ONLY ON" 
)
CPMAddPackage("gh:tinyobjloader/tinyobjloader#v2.0.0rc13")
CPMAddPackage("gh:google/googletest#v1.16.0")
CPMAddPackage(
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.4.310
        OPTIONS "VK_BOOTSTRAP_VULKAN_HEADER_DIR """
)
CPMAddPackage("gh:g-truc/glm#1.0.1")
CPMAddPackage("gh:gabime/spdlog#v1.15.1")

# lua, remove main executable source
CPMAddPackage(
        GITHUB_REPOSITORY lua/lua
        GIT_TAG v5.4.7
        DOWNLOAD_ONLY YES
)
FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/onelua.c")
add_library(lua STATIC ${lua_sources})
target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)

# sol2, do not depend on internal LUA! It's not stable
CPMAddPackage(
        GITHUB_REPOSITORY ThePhD/sol2
        GIT_TAG v3.5.0
        OPTIONS "SOL2_LUA_VERSION 5.4.7" "SOL2_BUILD_LUA FALSE"
)

# stb image
CPMAddPackage(
        GITHUB_REPOSITORY nothings/stb
        GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5
        DOWNLOAD_ONLY YES
)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE $<BUILD_INTERFACE:${stb_SOURCE_DIR}>)

# Imgui
CPMAddPackage(
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG v1.91.9
        DOWNLOAD_ONLY YES
)
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>)
target_sources(imgui PRIVATE
        ${imgui_SOURCE_DIR}/imgui.h
        ${imgui_SOURCE_DIR}/imgui.cpp

        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp

        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        )
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

