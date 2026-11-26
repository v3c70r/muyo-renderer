# muyo-renderer

The Muyo render pipeline and host, built on the [`muyo_rg`](https://github.com/v3c70r/Muyo) render
graph and `muyo_device` device libraries.

This repository is the consumer side of the Muyo library boundary: it owns the render pipeline
(GBuffer → lighting → UI → final), the window/present host, the scene and resource managers, the
renderer's asset loading, and — since the shader move in #318 — the Slang/GLSL sources and the sample
scenes. It contains no library code — `muyo_rg` and `muyo_device` stay in the Muyo repository, and
this repository consumes them as a submodule.

## Building

```sh
git submodule update --init --recursive
cmake -S . -B build
cmake --build build
```

Pass `-DMUYO_SOURCE_DIR=<path>` to build against a Muyo worktree instead of the submodule.

The app and the tests need the Vulkan SDK on the environment, because this repository now owns the
shader build: `glslangValidator` and `slangc` come from its `Bin` directory, and the SDK also supplies
the validation layer and loader.

On a Linux machine without Wayland development packages the GLFW configure needs the X11 path:

```sh
cmake -S . -B build -DGLFW_BUILD_WAYLAND=OFF
```

The X11 backend needs `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev` and `libxi-dev`.

**Do not run `git submodule update --remote`.** `thirdparty/tinygltf` and
`thirdparty/tinyobjloader` are pinned deliberately: tinygltf's `release` branch is now v3, which
renames the API and would break `SceneImporter`, and tinyobjloader's newer `fast_float` does not
compile at C++20 on MSVC. The migrations are tracked as
[#1](https://github.com/v3c70r/muyo-renderer/issues/1) and
[#21](https://github.com/v3c70r/muyo-renderer/issues/21).

## Tests

```sh
cmake -S . -B build -DMUYO_SOURCE_DIR=<muyo-checkout>   # optional
cmake --build build --target tests Shaders
cd build && ./tests
```

`tests` is the renderer-coupled half of Muyo's old suite, moved here with the pipeline. It is
top-level only: Catch2 is fetched at configure time. The build compiles `shaders/` into the build
directory and copies `assets/` beside the binary. The suite creates a physical device, so run it on a
GPU; `[test device]` records what it ran on. Two `VUID-vkWaitForFences-pFences-parameter` lines on Unix
are expected — the fork case deliberately passes a bad pointer to observe the abort — and heap-path
cases skip on a device without `VK_EXT_descriptor_heap`.

## Capture

See [`docs/Capture.md`](docs/Capture.md) for the RenderDoc capture recipe on a Wayland session and the
RenderDoc extension-filtering finding.

## Coding standard

The same as the Muyo repository — see `docs/CodingConventions.md`, `.clang-format` and `.clang-tidy`.
A change to the standard is proposed to both repositories.
