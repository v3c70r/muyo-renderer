# Changelog

All notable changes to `muyo-renderer` are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). The process contract is the Muyo repository's
`AGENTS.md` (this repository has no copy); a change that breaks a guarantee the library's
`docs/Consumers.md` names is listed here as well as there.

## [0.1.0] - unreleased

### Added

- **The standalone renderer repository** (#3). The host, the render pipeline
  (GBuffer → lighting → UI → final), the scene and resource managers, the renderer-coupled test suite
  with its `GraphicsTestEnv` fixture, and one app — `helloVulkan`, with `helloVulkanRG` folded into it —
  moved out of Muyo. The libraries `muyo_rg` and `muyo_device` are consumed as the `thirdparty/Muyo`
  submodule plus `add_subdirectory`; `-DMUYO_SOURCE_DIR=<path>` builds against a Muyo worktree instead.
- **The shader corpus and the sample assets** (#7). 59 shader files (7 GLSL, 17 `.slang`,
  20 `.heap.slang`, 15 shared headers) compile through a top-level-only `Shaders` target to 44 `.spv`
  in `<build>/shaders`, the working-directory-relative path the app and the suite load; 165 asset files
  copy beside the binaries. `docs/Capture.md` carries the RenderDoc recipes that moved with the app.
- **A `DebugUIOverlay` regression case** (#10). Constructs the overlay on the device fixture and checks
  the font texture and the atlas tex id; with `IMGUI_USER_CONFIG` missing from `muyo_renderer` it aborts
  on the same `imgui.h` assertion the app died on.
- **This changelog**, seeded by the 2026-11-26 post-split sync audit.

### Fixed

- **Debug `helloVulkan` aborted before its first frame** (#8, fixed in #10).
  `IMGUI_USER_CONFIG="ImGuiUserConfig.h"` was set on `imgui` and `tests` only; the config changes
  inline definitions in `imgui.h`, so the 12 other translation units that include it disagreed about
  `ImTextureID_Invalid` and the font-atlas `SetTexID` assert fired. The define now reaches every imgui
  consumer: `muyo_renderer` PUBLIC (its public headers include `imgui.h`), `imguizmo` and `imnodes`
  PRIVATE with the `src/` include path. Introduced by #3's squash, which restored two of the six
  define lines Muyo had.
- **The plain `.slang` rule did not rebuild on a shared-header edit** (#9, fixed in #11). Its `DEPENDS`
  named only the source file, so `forward.vert.slang`, `testWorldPos.vert.slang`,
  `prepareDrawCmdBuffer.comp.slang` and `testPrimary.rgen.slang` stayed stale after a header-only
  change while the 27 modules around them regenerated. The rule now depends on the shared-header glob,
  like the GLSL and heap rules; the compile commands and the 44 outputs are unchanged. Inherited from
  Muyo's `#`-commented `DEPENDS` line.
