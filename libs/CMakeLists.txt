# You should have Vulkan installed on your machine
find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
add_library(stb_image INTERFACE)

# Lua related because lua doesn't provide CMAKE
# Script reference: https://github.com/eliasdaler/cmake-lua-with-sol2-example/blob/main/third_party/CMakeLists.txt
# Rn it's pointing at v5.4.6, if the version bump up SOL_LUA_VERSION should change too
file(GLOB LUA_SRC_FILES lua/*.c)
add_library(lua ${LUA_SRC_FILES})
add_library(lua::lua ALIAS lua)
target_include_directories(lua PUBLIC lua)
if (WIN32)
        target_compile_definitions(lua PRIVATE LUA_USE_WINDOWS)
endif()
if (UNIX)
        target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
        target_link_libraries(lua PRIVATE dl)
endif()

### sol2
set(SOL2_LUA_VERSION "5.4.6" CACHE STRING "The version of Lua used")
set(SOL2_BUILD_LUA FALSE CACHE BOOL "Always build Lua, do not search for it in the system")
add_subdirectory(sol2)

# Include package with provided CMakeList
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(tinygltf)
add_subdirectory(tinyobjloader)
add_subdirectory(googletest)
set(VK_BOOTSTRAP_VULKAN_HEADER_DIR ${Vulkan_INCLUDE_DIRS} CACHE INTERNAL "" FORCE)
add_subdirectory(vkbootstrap)
add_subdirectory(glm)
add_subdirectory(spdlog)
add_subdirectory(sdl EXCLUDE_FROM_ALL)
add_subdirectory(joltphysics/Build)

# include path
target_include_directories(vma INTERFACE vma/include)
target_include_directories(stb_image INTERFACE stb)

# Imgui path
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC imgui)
target_sources(imgui PRIVATE
        imgui/imgui.h
        imgui/imgui.cpp

        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp

        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        )
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

