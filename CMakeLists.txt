cmake_minimum_required(VERSION 3.5)
project(youtube_cpp_engine VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if (NOT DEFINED ASSETS_DIR_NAME)
    set(ASSETS_DIR_NAME assets)
endif()

# library find functions
find_package(Vulkan REQUIRED)

# add external libraries
add_subdirectory(external/glm)
add_subdirectory(external/glfw)
add_subdirectory(external/SDL)
add_subdirectory(external/vk-bootstrap)
add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/entt)

file(GLOB SOURCES "src/*.cpp" "src/*/*.cpp" "src/*/*/*.cpp" "src/*/*/*/*.cpp")

add_library(${PROJECT_NAME} ${SOURCES} src/platform/sdl_window.cpp src/platform/sdl_window.h)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        glm
        # TODO: investigate moving entt out of the public exposure land
        EnTT
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        SDL2main
        SDL2-static
        glfw
        vk-bootstrap
        VulkanMemoryAllocator
        external/stb
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glm
        # TODO: investigate moving entt out of the public exposure land
        EnTT
    PRIVATE
        ${Vulkan_LIBRARIES}
        SDL2main
        SDL2-static
        glfw
        vk-bootstrap
        VulkanMemoryAllocator
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        ASSETS_DIR_NAME="${ASSETS_DIR_NAME}"
)

# SHADER COMPILATION

file (GLOB SHADERS shaders/*.frag shaders/*.vert)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders)

foreach(shader ${SHADERS})
    get_filename_component(SHADER_NAME ${shader} NAME)

    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} -c ${shader} -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME}/shaders/${SHADER_NAME}.spv)
endforeach()