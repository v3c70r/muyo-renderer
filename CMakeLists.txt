# muyo-renderer — the Muyo render pipeline and host, built on the muyo_rg + muyo_device libraries.
#
# Muyo is consumed as a submodule plus add_subdirectory (the same way muyo-llm does it);
# MUYO_SOURCE_DIR overrides the checkout for co-development against a Muyo worktree. Coding
# conventions, the C++20 floor and the warning policy match the Muyo repository —
# see docs/CodingConventions.md.

cmake_minimum_required(VERSION 3.11)
# The language level is a directory default declared before project(), so every later-created target -
# including the Muyo subdirectory and any third-party subdirectories - inherits C++20, exactly as Muyo's
# own file does.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(muyo_renderer VERSION 0.1.0)

# Root of this project, correct whether it is the top-level build or a subdirectory. Mirrors Muyo's
# MUYO_ROOT / MUYO_BINARY_ROOT, which use CMAKE_CURRENT_LIST_DIR for the same reason (#68).
set(MUYO_RENDERER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(MUYO_RENDERER_BINARY_ROOT "${CMAKE_CURRENT_BINARY_DIR}")

# Runtime artefacts for this project's own targets. Muyo sets its own inside its directory scope, so
# its shaders land under `<build>/muyo/shaders/` while the renderer's executables land here.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# True when this project is the root of the build. The test suite and the demo apps are development
# concerns, so an embedder of the renderer does not get them.
set(MUYO_RENDERER_TOP_LEVEL FALSE)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(MUYO_RENDERER_TOP_LEVEL TRUE)
endif()

# ── Muyo, the libraries under the renderer ───────────────────────────────────
# The submodule is the shipping path. MUYO_SOURCE_DIR overrides it for local work against a
# development checkout, so iterating on Muyo itself does not require re-initialising submodules.
set(MUYO_SOURCE_DIR "" CACHE PATH "Muyo checkout to build against (default: thirdparty/Muyo)")
if (NOT MUYO_SOURCE_DIR)
    set(MUYO_SOURCE_DIR "${MUYO_RENDERER_ROOT}/thirdparty/Muyo")
endif()
if (NOT EXISTS "${MUYO_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Muyo not found at ${MUYO_SOURCE_DIR}. Initialise the submodule "
        "(git submodule update --init --recursive) or pass -DMUYO_SOURCE_DIR=<path>.")
endif()
add_subdirectory("${MUYO_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/muyo")

# ── Compiler warnings, scoped to this project's targets ──────────────────────
# The same policy as Muyo's own targets: -Wall -Wextra, with -Wno-missing-field-initializers because
# designated initialisers omit fields on purpose (docs/CodingConventions.md bans positional
# initialisation of Vulkan structs, and the check reports exactly that required style).
if (NOT MSVC)
    add_compile_options(-Wall -Wextra -Wno-missing-field-initializers -Wno-missing-braces -pthread)
    add_link_options(-pthread)
else()
    # Windows builds mirror Muyo's MSVC flags.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /W3")
endif()

# ── Deprecation enforcement ──────────────────────────────────────────────────
# The library signals a breaking change through [[deprecated("use X")]]; forcing the warning into an
# error means the renderer is told by its own compiler, exactly as Muyo's own targets are.
function(muyo_renderer_enable_deprecation_errors target)
    if (UNIX)
        target_compile_options(${target} PRIVATE -Werror=deprecated-declarations)
    elseif (MSVC)
        target_compile_options(${target} PRIVATE /we4996)
    endif()
endfunction()

# ── Third-party dependencies ─────────────────────────────────────────────────
# The renderer owns its window, UI, model and asset dependencies. Muyo's device layer is
# `muyo_device`; the renderer adds only what a host and a render pipeline need.

## GLFW
find_package(glfw3 3.2 QUIET)
if (NOT ${glfw3_FOUND})
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    # Both Unix backends, for the same reason Muyo builds them: RenderDoc's capture launcher under a
    # Wayland session needs GLFW's X11 backend (AGENTS.md section 4, #220).
    if (UNIX AND NOT APPLE)
        set(GLFW_BUILD_X11 ON CACHE BOOL "Build GLFW's X11 backend (RenderDoc capture selects it via Xwayland)")
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build GLFW's Wayland backend")
    endif()
    add_subdirectory(${MUYO_RENDERER_ROOT}/thirdparty/glfw)
    if (UNIX AND NOT APPLE AND NOT GLFW_BUILD_X11)
        message(STATUS
            "GLFW's X11 backend is disabled; RenderDoc capture on a Wayland session needs it. "
            "Reconfigure with -DGLFW_BUILD_X11=ON (see Muyo's AGENTS.md section 4).")
    endif()
endif()
add_compile_definitions(USE_GLFW)
set(WINDOW_LIB glfw)

## GLM (header-only)
find_package(glm QUIET)
if (NOT ${glm_FOUND})
    set(GLM_INCLUDE_DIRS ${MUYO_RENDERER_ROOT}/thirdparty/glm)
endif()

## imgui
set(IMGUI_INCLUDE_DIRS ${MUYO_RENDERER_ROOT}/thirdparty/imgui)
add_library(imgui
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imgui_tables.cpp
    )
# Use a project-local imgui config (ImTextureID_Invalid override) instead of editing the submodule.
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_INCLUDE_DIRS})
target_include_directories(imgui PRIVATE ${MUYO_RENDERER_ROOT}/src)
target_compile_definitions(imgui PRIVATE IMGUI_USER_CONFIG="ImGuiUserConfig.h")

## ImGuizmo
# ImGuizmo.cpp includes "imgui.h" and "imgui_internal.h"; Muyo supplied those through a
# directory-scoped include_directories(SYSTEM ...), which does not exist here, so the edge is stated.
set(IMGUIZMO_INCLUDE_DIRS ${MUYO_RENDERER_ROOT}/thirdparty/ImGuizmo/src)
add_library(imguizmo ${IMGUIZMO_INCLUDE_DIRS}/ImGuizmo.cpp)
target_link_libraries(imguizmo PRIVATE imgui)

## imnodes
# imnodes.h includes "imgui.h" in its public interface, so the dependency is PUBLIC here.
set(IMNODES_INCLUDE_DIRS ${MUYO_RENDERER_ROOT}/thirdparty/imnodes)
add_library(imnodes thirdparty/imnodes/imnodes.cpp)
target_link_libraries(imnodes PUBLIC imgui)

## Vulkan (the library targets already require it; the renderer names it for its own link line)
if (NOT ENV{VULKAN_SDK} AND EXISTS ENV{VULKAN_SDK})
    list(APPEND CMAKE_PREFIX_PATH $ENV{VULKAN_SDK})
endif()
find_package(Vulkan REQUIRED)

## tinyobjloader
include_directories(${MUYO_RENDERER_ROOT}/thirdparty/tinyobjloader)
add_library(tinyobj ${MUYO_RENDERER_ROOT}/thirdparty/tinyobjloader/tiny_obj_loader.cc)

## tinygltf
# Pinned to the last v2 release: upstream's `release` branch is now v3, which renames the whole API
# (tiny_gltf_v3.c/h) and would break SceneImporter. The v3 migration is filed separately.
#
# No `tinygltf` library target: upstream's `tiny_gltf.cc` also defines STB_IMAGE_IMPLEMENTATION, which
# instantiates a second stb_image and collides with the single implementation in
# `src/RenderResources/stb_image_impl.cpp`. The implementation TU lives in
# `src/RenderResources/tinygltf_impl.cpp` and is compiled into `muyo_renderer` by the src glob, the same
# way stb's is.
include_directories(${MUYO_RENDERER_ROOT}/thirdparty/tinygltf)

## stb (nothings/stb submodule). Only stb_image is used; its single implementation TU is
## `src/RenderResources/stb_image_impl.cpp`, compiled into muyo_renderer by the src glob below.
include_directories(${MUYO_RENDERER_ROOT}/thirdparty/stb)

## meshoptimizer
set(MESHOPT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${MUYO_RENDERER_ROOT}/thirdparty/meshoptimizer)
include_directories(${MUYO_RENDERER_ROOT}/thirdparty/meshoptimizer/src)

# ── The renderer ─────────────────────────────────────────────────────────────
# The engine's former `muyo` target, minus the libraries: host, scene, resources and debug UI. The two
# bottom layers come from Muyo as `muyo_rg` + `muyo_device` (#318).
file(GLOB MUYO_RENDERER_SRC
    "src/*.h"
    "src/*.cpp"
    "src/Debug/*.h"
    "src/Debug/*.cpp"
    "src/RenderResources/*.h"
    "src/RenderResources/*.cpp"
    "src/Scene/*.h"
    "src/Scene/*.cpp"
    )

add_library(muyo_renderer STATIC ${MUYO_RENDERER_SRC})
target_include_directories(muyo_renderer
    PUBLIC
        ${MUYO_RENDERER_ROOT}/src
        ${MUYO_RENDERER_ROOT}/src/RenderResources
        ${MUYO_RENDERER_ROOT}/src/Scene
        ${MUYO_RENDERER_ROOT}/src/Debug
        # Shader-side shared structs (`SharedStructures.h`) are still in the Muyo checkout, because the
        # `Shaders` target that compiles against them is Muyo's; the C++ side has to read the same
        # header or the two sides drift. Moving the shaders here is part of #318.
        ${MUYO_SOURCE_DIR}/shaders/shared
        ${GLM_INCLUDE_DIRS}
        ${IMGUI_INCLUDE_DIRS}
        ${IMGUIZMO_INCLUDE_DIRS}
        ${IMNODES_INCLUDE_DIRS}
        ${MUYO_RENDERER_ROOT}/thirdparty/tinyobjloader
        ${MUYO_RENDERER_ROOT}/thirdparty/tinygltf
        ${MUYO_RENDERER_ROOT}/thirdparty/stb
        ${MUYO_RENDERER_ROOT}/thirdparty/meshoptimizer/src
        ${MUYO_RENDERER_ROOT}/thirdparty/RenderDoc
    )
# Include directories that a plain include_directories() call above would supply to the library's own
# translation units; kept separate so consumers of muyo_renderer do not inherit all of them.
target_include_directories(muyo_renderer SYSTEM
    PUBLIC
        $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>
    )

target_compile_features(muyo_renderer PUBLIC cxx_std_20)
target_link_libraries(muyo_renderer
    PUBLIC
        muyo_rg
        muyo_device
        # `src/WindowGLFW.h` is part of this library's public include surface and includes
        # `GLFW/glfw3.h`, so a consumer of the host layer needs GLFW's headers and its link edge.
        # Muyo's engine target supplied this with the plain (PUBLIC-by-default) link signature.
        ${WINDOW_LIB}
    PRIVATE
        imgui
        imguizmo
        imnodes
        tinyobj
        meshoptimizer
        ${Vulkan_LIBRARIES}
        VulkanMemoryAllocator
        ${GLM_INCLUDE_DIRS}
    )
muyo_renderer_enable_deprecation_errors(muyo_renderer)

# ── Applications ─────────────────────────────────────────────────────────────
if (MUYO_RENDERER_TOP_LEVEL)
    add_executable(helloVulkan src/app/helloVulkan.cpp)
    target_link_libraries(helloVulkan PRIVATE muyo_renderer)
    muyo_renderer_enable_deprecation_errors(helloVulkan)

    add_executable(helloVulkanRG src/app/helloVulkanRG.cpp)
    target_link_libraries(helloVulkanRG PRIVATE muyo_renderer)
    muyo_renderer_enable_deprecation_errors(helloVulkanRG)

    # Shaders are built by the Muyo subdirectory's unguarded `Shaders` target into `<build>/muyo/shaders`.
    # The apps load `<name>.spv` from `shaders/` relative to the working directory, so the directory is
    # copied next to this project's executables after each app build.
    foreach (app helloVulkan helloVulkanRG)
        add_dependencies(${app} Shaders)
        add_custom_command(TARGET ${app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_BINARY_DIR}/muyo/shaders"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
    endforeach()

    # Assets still live in the Muyo checkout for now; the renderer loads `assets/` relative to the
    # working directory. Moving them is part of the renderer split (#318).
    set(MUYO_RENDERER_ASSETS "${MUYO_SOURCE_DIR}/assets")
    if (EXISTS "${MUYO_RENDERER_ASSETS}")
        add_custom_command(TARGET helloVulkan POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${MUYO_RENDERER_ASSETS}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")
    endif()
endif()
