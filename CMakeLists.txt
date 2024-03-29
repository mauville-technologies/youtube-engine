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
add_subdirectory(external/json)
add_subdirectory(external/SPRIV-Cross)
add_subdirectory(external/assimp)
add_subdirectory(external/openxr)

add_library(${PROJECT_NAME} "")

target_sources(${PROJECT_NAME}
    PRIVATE
        src/core/entity.cpp
        src/core/game.cpp
        src/core/scene.cpp
        src/core/components/camera_component.cpp
        src/core/components/mesh_component.cpp
        src/core/components/transform_component.cpp

        src/input/input_manager.cpp
        src/input/multiplatform_input.cpp
        src/input/sdl_input.cpp

        src/platform/configuration_manager.cpp
        src/platform/filesystem.cpp
        src/platform/multiplatform_window.cpp
        src/platform/sdl_window.cpp

        src/rendering/images.cpp
        src/rendering/stbi.cpp
        src/rendering/vulkan/vulkan_buffer.cpp
        src/rendering/vulkan/vulkan_descriptor_set_manager.cpp
        src/rendering/vulkan/vulkan_includes.h
        src/rendering/vulkan/vulkan_initializers.cpp
        src/rendering/vulkan/vulkan_pipeline_builder.cpp
        src/rendering/vulkan/vulkan_renderer.cpp
        src/rendering/vulkan/vulkan_shader.cpp
        src/rendering/vulkan/vulkan_texture.cpp
        src/rendering/vulkan/vulkan_utilities.cpp

        src/vr/openxr/open_xr_subsystem.cpp

        src/resources/resource_manager.cpp
        src/resources/types/image.cpp
        src/resources/types/material.cpp
        src/resources/types/mesh.cpp
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            OZZ_WINDOWS
    )
endif()

if (APPLE)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            OZZ_MAC
    )
endif()

if (UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            OZZ_LINUX
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        glm
        nlohmann_json

        # TODO: investigate moving entt out of the public exposure land
        EnTT
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        SDL3-static
    SYSTEM
        glfw
        vk-bootstrap
        VulkanMemoryAllocator
        external/stb
        SPIRV-Cross
        assimp
        openxr_loader
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glm
        nlohmann_json

        # TODO: investigate moving entt out of the public exposure land
        EnTT
    PRIVATE
        ${Vulkan_LIBRARIES}
        SDL3-static
        glfw
        vk-bootstrap
        VulkanMemoryAllocator
        spirv-cross-core
        spirv-cross-glsl
        assimp
        openxr_loader
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

if (DEFINED ASSETS_DIR_NAME)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/defaultAssets/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ASSETS_DIR_NAME})
endif()

add_executable(openxr_sandbox sandbox/openxr/main.cpp)

target_include_directories(openxr_sandbox
    PRIVATE
        openxr_loader
        ${Vulkan_INCLUDE_DIRS}
        glm
)

target_link_libraries(openxr_sandbox
    PRIVATE
        ${Vulkan_LIBRARIES}
        openxr_loader
        glm
)