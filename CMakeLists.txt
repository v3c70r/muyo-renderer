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
# the libraries' artefacts land under `<build>/muyo/` while the renderer's executables, its compiled
# shaders and its copied assets land here.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${MUYO_RENDERER_BINARY_ROOT})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# True when this project is the root of the build. The test suite and the demo app are development
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
if (DEFINED ENV{VULKAN_SDK})
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
        # Shader-side shared structs (`SharedStructures.h`). They moved here with the shader corpus
        # (#318); the C++ side has to read the same header the `.slang` files include or the two sides
        # drift.
        ${MUYO_RENDERER_ROOT}/shaders/shared
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
    # ── Shaders ──────────────────────────────────────────────────────────────
    # The corpus moved here from Muyo with the pipeline (#318). It used to be compiled by Muyo's
    # `Shaders` target into `<build>/muyo/shaders` and copied beside the binaries; this project now
    # owns the target and writes directly to `<build>/shaders`, the working-directory-relative path
    # the app and the suite load. Top-level only: an embed of the renderer library needs neither the
    # compilers nor the `.spv` files.
    if (NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        message(FATAL_ERROR "Can't find glslangValidator (the Vulkan SDK's Bin directory must be on PATH)")
    endif()
    if (NOT Vulkan_SLANG_EXECUTABLE)
        find_program(Vulkan_SLANG_EXECUTABLE
            NAMES slangc
            HINTS "$ENV{VULKAN_SDK}/bin")
    endif()
    if (NOT Vulkan_SLANG_EXECUTABLE)
        message(FATAL_ERROR "Can't find slangc (the Vulkan SDK's Bin directory must be on PATH)")
    endif()

    set(MUYO_RENDERER_SHADER_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")

    # GLSL sources. The header glob is a build dependency of every one of them: a shared struct change
    # has to reach the modules that include it.
    file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${MUYO_RENDERER_ROOT}/shaders/*.frag"
        "${MUYO_RENDERER_ROOT}/shaders/*.vert"
        "${MUYO_RENDERER_ROOT}/shaders/*.rgen"
        "${MUYO_RENDERER_ROOT}/shaders/*.rmiss"
        "${MUYO_RENDERER_ROOT}/shaders/*.rchit"
        "${MUYO_RENDERER_ROOT}/shaders/*.comp"
        )
    file(GLOB_RECURSE GLSL_HEADER_FILES "${MUYO_RENDERER_ROOT}/shaders/*.h")

    foreach (GLSL_SOURCE_FILE ${GLSL_SOURCE_FILES})
        get_filename_component(FILE_NAME ${GLSL_SOURCE_FILE} NAME)
        set(SPIRV "${MUYO_RENDERER_SHADER_DIR}/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MUYO_RENDERER_SHADER_DIR}"
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} --target-env vulkan1.3 -DSHADER_CODE -g -V ${GLSL_SOURCE_FILE} -o ${SPIRV}
            DEPENDS ${GLSL_SOURCE_FILE} ${GLSL_HEADER_FILES})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach()

    # Slang sources. The `.heap.slang` variants are built by the rule below instead, so each source is
    # compiled exactly once (docs/DescriptorHeap-Bindless-Design.md §9).
    file(GLOB_RECURSE SLANG_SOURCE_FILES "${MUYO_RENDERER_ROOT}/shaders/*.slang")
    list(FILTER SLANG_SOURCE_FILES EXCLUDE REGEX "\\.heap\\.slang$")
    foreach (SLANG_SOURCE_FILE ${SLANG_SOURCE_FILES})
        get_filename_component(FILE_NAME ${SLANG_SOURCE_FILE} NAME)
        set(SPIRV "${MUYO_RENDERER_SHADER_DIR}/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MUYO_RENDERER_SHADER_DIR}"
            COMMAND ${Vulkan_SLANG_EXECUTABLE} ${SLANG_SOURCE_FILE} -DSHADER_CODE -DSLANG
                    -capability spirv_1_4 -profile glsl_450 -target spirv -o ${SPIRV} -entry main
            DEPENDS ${SLANG_SOURCE_FILE})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach()

    # Descriptor-heap variants. `-capability spvDescriptorHeapEXT` emits SPV_EXT_descriptor_heap +
    # SPV_KHR_untyped_pointers; the heap strides stay at Slang's default 0 so the module carries
    # OpConstantSizeOfEXT/ArrayStrideIdEXT rather than a device-specific stride
    # (docs/DescriptorHeap-Bindless-Design.md §5). The output keeps the `.heap` marker so a node can
    # name `foo.heap` while the non-heap path names `foo`.
    file(GLOB_RECURSE HEAP_SLANG_SOURCE_FILES "${MUYO_RENDERER_ROOT}/shaders/*.heap.slang")
    # Prefer the SDK's own validator: a distribution's spirv-val can be older than the extension it is
    # asked to validate. NO_DEFAULT_PATH makes the build use the pinned SDK or nothing.
    find_program(SPIRV_VAL_EXECUTABLE
        NAMES spirv-val
        HINTS "$ENV{VULKAN_SDK}/bin"
        NO_DEFAULT_PATH)
    foreach (HEAP_SLANG_SOURCE_FILE ${HEAP_SLANG_SOURCE_FILES})
        get_filename_component(FILE_NAME ${HEAP_SLANG_SOURCE_FILE} NAME)
        string(REGEX REPLACE "\\.slang$" ".spv" SPIRV_NAME "${FILE_NAME}")
        set(SPIRV "${MUYO_RENDERER_SHADER_DIR}/${SPIRV_NAME}")
        set(HEAP_VALIDATE_COMMAND)
        if (SPIRV_VAL_EXECUTABLE)
            set(HEAP_VALIDATE_COMMAND COMMAND ${SPIRV_VAL_EXECUTABLE} --target-env vulkan1.4 ${SPIRV})
        else()
            # Say it in configure output rather than let an unvalidated `.heap.spv` look valid.
            message(STATUS "descriptor-heap .spv validation: disabled (no spirv-val in $ENV{VULKAN_SDK}/bin)")
        endif()
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MUYO_RENDERER_SHADER_DIR}"
            COMMAND ${Vulkan_SLANG_EXECUTABLE} ${HEAP_SLANG_SOURCE_FILE} -DSHADER_CODE -DSLANG
                    -capability spvDescriptorHeapEXT -capability spirv_1_4
                    -profile glsl_450 -target spirv -o ${SPIRV} -entry main
            ${HEAP_VALIDATE_COMMAND}
            DEPENDS ${HEAP_SLANG_SOURCE_FILE} ${GLSL_HEADER_FILES})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach()

    add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

    # One app. `helloVulkanRG` was folded into it: the graph path was already this app's frame loop, and
    # the trace page it uniquely had is now registered with the debug overlay (#318).
    add_executable(helloVulkan src/app/helloVulkan.cpp)
    target_link_libraries(helloVulkan PRIVATE muyo_renderer)
    muyo_renderer_enable_deprecation_errors(helloVulkan)
    add_dependencies(helloVulkan Shaders)

    # The app and the tests load `assets/` relative to the working directory, so the tree is copied
    # beside the executable. It is this repository's own `assets/` now (#318).
    set(MUYO_RENDERER_ASSETS "${MUYO_RENDERER_ROOT}/assets")
    add_custom_command(TARGET helloVulkan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MUYO_RENDERER_ASSETS}"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

    # ── Tests ─────────────────────────────────────────────────────────────────
    # A top-level-only development concern, exactly as in Muyo: Catch2 is fetched over the network at
    # configure time, so an embedder of the renderer does not pay for it. The suite is the
    # renderer-coupled half of Muyo's old suite, moved with the pipeline (#318); `PSODescTest` stays in
    # Muyo because its subject is the library.
    enable_testing()
    include(CTest)

    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.0)
    FetchContent_MakeAvailable(Catch2)
    message(STATUS "Catch2 source dir: ${catch2_SOURCE_DIR}")

    add_executable(tests
        src/tests/ArcballTest.cpp
        src/tests/DebugCallbackTests.cpp
        src/tests/DescriptorHeapProbeTests.cpp
        src/tests/DescriptorHeapServiceTests.cpp
        src/tests/DeviceCapabilityTests.cpp
        src/tests/FrameSyncTest.cpp
        src/tests/RenderGraphBuilderTest.cpp
        src/tests/RenderGraphFinalNodeTest.cpp
        src/tests/RenderGraphGBufferLightingTest.cpp
        src/tests/RenderGraphHeapTests.cpp
        src/tests/RenderGraphPipelineCacheTests.cpp
        src/tests/RenderGraphShaderCacheTests.cpp
        src/tests/RenderGraphTracePageSelectionTests.cpp
        src/tests/SceneBoundsTest.cpp
        src/tests/SceneImporterTests.cpp
        src/tests/ShaderReflectionTests.cpp
        )
    target_include_directories(tests PRIVATE
        ${catch2_SOURCE_DIR}/src
        ${MUYO_RENDERER_ROOT}/thirdparty/RenderDoc)
    # `muyo_renderer` already provides every renderer source and include this binary needs, and it links
    # the libraries, so the suite names only the renderer target and the Catch2 main.
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain muyo_renderer)
    # Keep ImTextureID_Invalid consistent with the imgui target (see src/ImGuiUserConfig.h).
    target_compile_definitions(tests PRIVATE IMGUI_USER_CONFIG="ImGuiUserConfig.h")
    muyo_renderer_enable_deprecation_errors(tests)

    # The suite reads the compiled shaders and `assets/` relative to the working directory, and ctest
    # runs it from the build directory. `RenderGraphBuilderTest` fails without
    # `assets/mazda_mx-5_spot/untitled.gltf`, so the asset copy is not optional. The shaders are already
    # in `<build>/shaders` because the target writes there directly (#318).
    add_dependencies(tests Shaders)
    add_custom_command(TARGET tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MUYO_RENDERER_ASSETS}"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

    include(Catch)
    catch_discover_tests(tests)
endif()
