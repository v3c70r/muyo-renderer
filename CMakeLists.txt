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

find_package(Vulkan REQUIRED)

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
# The renderer owns its window/UI/model/asset dependencies (glfw, glm, imgui, ImGuizmo, imnodes,
# meshoptimizer, tinygltf, tinyobjloader, stb). They are declared here as the corresponding sources
# move over from Muyo (#318).

# ── Renderer targets ─────────────────────────────────────────────────────────
# Filled in as the render pipeline, host and scene sources move over from Muyo (#318).
