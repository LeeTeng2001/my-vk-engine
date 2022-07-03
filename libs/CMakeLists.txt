# Add custom find path for window mingw sdl2
message(STATUS "Adding custom sdl2 path for window...")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/sdl2/")

# You should have Vulkan and SDL2 installed on your machine
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_library(vma INTERFACE)
add_library(stb_image INTERFACE)

# Include package with provided CMakeList
add_subdirectory(vkbootstrap)
add_subdirectory(glm)

# include path
target_include_directories(vma INTERFACE vma/include)
target_include_directories(stb_image INTERFACE stb)

# SDL2 path
add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
target_link_libraries(sdl2 INTERFACE ${SDL2_LIBRARIES})

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
        imgui/backends/imgui_impl_sdl.cpp
        )
target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)

