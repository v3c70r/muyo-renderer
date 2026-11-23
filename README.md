# muyo-renderer

The Muyo render pipeline and host, built on the [`muyo_rg`](https://github.com/v3c70r/Muyo) render
graph and `muyo_device` device libraries.

This repository is the consumer side of the Muyo library boundary: it owns the render pipeline
(GBuffer → lighting → UI → final), the window/present host, the scene and resource managers, and the
renderer's asset loading. It contains no library code — `muyo_rg` and `muyo_device` stay in the Muyo
repository, and this repository consumes them as a submodule.

## Building

```sh
git submodule update --init --recursive
cmake -S . -B build
cmake --build build
```

Pass `-DMUYO_SOURCE_DIR=<path>` to build against a Muyo worktree instead of the submodule.

## Coding standard

The same as the Muyo repository — see `docs/CodingConventions.md`, `.clang-format` and `.clang-tidy`.
A change to the standard is proposed to both repositories.
