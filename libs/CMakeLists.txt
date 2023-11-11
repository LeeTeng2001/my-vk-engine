# You should have Vulkan installed on your machine
find_package(Vulkan REQUIRED)

add_library(vma INTERFACE)
add_library(stb_image INTERFACE)

# Include package with provided CMakeList
add_subdirectory(vkbootstrap)
add_subdirectory(glm)
add_subdirectory(sdl EXCLUDE_FROM_ALL)

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

